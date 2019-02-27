/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include <algorithm>
#include "../include/dexlize.hpp"

eosio::asset Dexlize::Network::_toAsset(const string& amount) {
    auto pos = amount.find(" ");
    eosio_assert(pos != string::npos, "the format of aseet is not correct");
    int64_t quantity = static_cast<int64_t>(stoi(amount.substr(0, pos)) * 10000);
    eosio_assert(quantity > 1, "the order amount must greater than zero");
    return asset(quantity, string_to_symbol(4, amount.substr(pos + 1).c_str()));
}

bool Dexlize::Network::_parseMemo(const map<string, string>& memoMap, uint64_t& id, account_name& contract) {
    bool reVal = false;

    auto id_ptr = memoMap.find("id");
    eosio_assert(id_ptr != memoMap.end(), "must set order id in the memo when buying or selling");
    id = stoi(id_ptr->second);

    auto contract_ptr = memoMap.find("contract");
    eosio_assert(contract_ptr != memoMap.end(), "must set the contract in the memo");
    contract = string_to_name(contract_ptr->second.c_str());

    return true;
}

bool Dexlize::Network::_parseMemo(const map<string, string>& memoMap, asset& exchanged, asset& exchange, account_name& contract) {
    bool reVal = false;

    auto exchanged_ptr = memoMap.find("exed");
    eosio_assert(exchanged_ptr != memoMap.end(), "must set the token asset exchanged in the memo");
    exchanged = _toAsset(exchanged_ptr->second);

    auto exchange_ptr = memoMap.find("ex");
    eosio_assert(exchange_ptr != memoMap.end(), "must set the EOS asset exchange in the memo");
    exchange = _toAsset(exchange_ptr->second);

    auto contract_ptr = memoMap.find("contract");
    eosio_assert(contract_ptr != memoMap.end(), "must set the contract in the memo");
    contract = string_to_name(contract_ptr->second.c_str());

    return true;
}

void Dexlize::Network::_sell(const account_name& from, const extended_asset& quantity, const account_name& contract, uint64_t id) {
    tb_buys buys(_self, contract);
    auto buy_ptr = buys.find(id);
    eosio_assert(buy_ptr != buys.end(), "the buy order is not exist");
    eosio_assert((buy_ptr->exchanged - quantity).amount >= 0, "the quantity brought must be less than amount of order");
    auto fee = quantity.amount * get_taker_ratio() / 1000;
    int64_t sell_amount(static_cast<double>(quantity.amount - fee) / static_cast<double>(buy_ptr->exchanged.amount) * buy_ptr->exchange.amount);

    if (buy_ptr->exchanged.amount == (quantity.amount - fee)) {
        buys.erase(buy_ptr);
        tb_accounts accounts(_self, from);
        auto acc_ptr = accounts.find(buy_ptr->name);
        accounts.modify(acc_ptr, 0, [&](auto& a) {
            auto acc_buy_ptr = find(a.buys.end(), a.buys.end(), id);
            eosio_assert(acc_buy_ptr != a.buys.end(), "buy order id is not exist in the account table");
            a.buys.erase(acc_buy_ptr);
        });
    } else {
        buys.modify(buy_ptr, 0, [&](auto& a) {
            a.exchanged -= asset(quantity.amount - fee, quantity.symbol);
            a.exchange.amount -= sell_amount;
        });
    }

    action(permission_level{_self, N(active)},
           buy_ptr->exchange.contract,
           N(transfer),
           make_tuple(_self, from, asset(sell_amount, buy_ptr->exchange.symbol), string("sell token, exchange: dexlize network"))).send();

    action(permission_level{_self, N(active)},
        buy_ptr->exchanged.contract,
        N(transfer),
        make_tuple(_self, buy_ptr->name, asset(quantity.amount - fee, buy_ptr->exchanged.symbol), string("buy token, exchange: dexlize network"))).send();
}

void Dexlize::Network::_buy(const account_name& from, const extended_asset& quantity, const account_name& contract, uint64_t id) {
    tb_sells sells(_self, contract);
    auto sell_ptr = sells.find(id);
    eosio_assert(sell_ptr != sells.end(), "the sell order is not exist");
    eosio_assert((sell_ptr->exchange - quantity).amount >= 0, "the quantity selled must be less than amount of order");
    auto fee = quantity.amount * get_taker_ratio() / 1000;
    int64_t sell_amount(static_cast<double>(quantity.amount - fee) / static_cast<double>(sell_ptr->exchange.amount) * sell_ptr->exchanged.amount);
    if (sell_ptr->exchange.amount == (quantity.amount - fee)) {
        sells.erase(sell_ptr);
        tb_accounts accounts(_self, from);
        auto acc_ptr = accounts.find(sell_ptr->exchanged.contract);
        eosio_assert(acc_ptr != accounts.end(), "the contract address is not exist in the account table");
        accounts.modify(acc_ptr, 0, [&](auto& a) {
            auto acc_sell_ptr = find(a.sells.end(), a.sells.end(), id);
            eosio_assert(acc_sell_ptr != a.sells.end(), "sell order id is not exist in the account table");
            a.sells.erase(acc_sell_ptr);
        });
    } else {
        sells.modify(sell_ptr, 0, [&](auto& a) {
            a.exchanged.amount -= sell_amount;
            a.exchange -= asset(quantity.amount - fee, quantity.symbol);
        });
    }

    action(permission_level{_self, N(active)},
           sell_ptr->exchanged.contract,
           N(transfer),
           make_tuple(_self, from, asset(sell_amount, sell_ptr->exchanged.symbol), string("buy token, exchange: dexlize network"))).send();

    action(permission_level{_self, N(active)},
        sell_ptr->exchange.contract,
        N(transfer),
        make_tuple(_self, sell_ptr->name, asset(quantity.amount - fee, sell_ptr->exchange.symbol), string("sell token, exchange: dexlize network"))).send();
}

void Dexlize::Network::_activeSellOrder(const extended_asset& quantity, const account_name& contract, uint64_t id) {
    tb_sells sells(_self, contract);
    auto sell_ptr = sells.find(id);
    eosio_assert(sell_ptr != sells.end(), "current sell order is not exist");
    auto fee = quantity.amount * get_maker_ratio() / 1000;
    eosio_assert(sell_ptr->exchanged.amount == (quantity.amount - fee), "transfer quantity must be equal with exchanged quantity");
    sells.modify(sell_ptr, 0, [&](auto& a) {
        a.actived = true;
    });
}

void Dexlize::Network::_activeBuyOrder(const extended_asset& quantity, const account_name& contract, uint64_t id) {
    eosio_assert(quantity.contract == N(eosio.token), "must pay with EOS token by eosio.token");
    eosio_assert(quantity.symbol == EOS_SYMBOL, "must pay with EOS token");
    tb_buys buys(_self, contract);
    auto buy_ptr = buys.find(id);
    eosio_assert(buy_ptr != buys.end(), "current buy order is not exist");
    auto fee = quantity.amount * get_maker_ratio() / 1000;
    eosio_assert(buy_ptr->exchange.amount == (quantity.amount - fee), "transfer quantity must be equal with exchange quantity");
    buys.modify(buy_ptr, 0, [&](auto& a) {
        a.actived = true;
    });
}

/**
 * function: display the current contract version
 * parameter: void
 */
void Dexlize::Network::version()
{
    require_auth(_self);
}

/**
 * function: user can transfer the eos to buy take by this interface, and also can set the stake beneficiary
 * parameter: memo - json format e.g. {"contract": "dexlize", "symbol": "DEX", "owner": "eosbitportal"}
 * */
void Dexlize::Network::buy(account_name from, account_name to, asset eos, string memo)
{
    // // check if the account name is correct.
    // // from account cannot be owner and to account must be owner.
    // if (from == _self || to != _self) return;
    // eosio_assert(eos.symbol == CORE_SYMBOL, "must pay with CORE token");

    // // take the symbol name, and the check if the symbol name is in the memo
    // Utils utils;
    // map<string, string> memoMap;
    // eosio_assert(!utils.parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    // account_name contractAccount = aux.getContractAccount(memoMap);
    // symbol_name symbolName = aux.getSymbolName(memoMap);
    // eosio_assert(_checkSymbol(contractAccount, symbolName), "current symbol is not supported");

    // // get account of contract by the different symbol name in the memo
    // // construct the memo of action by the different symbol name in the memo
    // // and then execute the transfer action of target contract
    // string owner = aux.getOwnerAccount(memoMap);
    // string actionMemo = aux.getActionMemo(symbolName, owner);
    // _sendAction(N(eosio.token), contractAccount, eos, ACTION_TRANSFER_TYPE, actionMemo);
}

/**
 * function: user can sell stake to gain the eos, and also can set the selled stake beneficiary
 * paraneter: memo - json format e.g. {"owner": "eosbitportal"}
 **/
void Dexlize::Network::sell(account_name from, account_name to, asset quantity, string memo)
{
    require_auth(from);

    // // check the account of from and to
    // if (from == _self && to != _self) return;
    // eosio_assert(quantity.is_valid(), "invalid quantity");
    // eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    // eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    // // parse the memo of json fomat to get the transfer type
    // Utils utils;
    // map<string, string> memoMap;
    // eosio_assert(!utils.parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    // account_name contractAccount = aux.getContractAccount(memoMap);
    // symbol_name symbolName = aux.getSymbolName(memoMap);
    // eosio_assert(_checkSymbol(contractAccount, symbolName), "current symbol is not supported");

    // // get the contract account from the asset symbol
    // // execute the action of sell by the target contract
    // string owner = aux.getOwnerAccount(memoMap); 
    // _sendAction(contractAccount, contractAccount, quantity, ACTION_SELL_TYPE, "-owner:" + owner);
}

void Dexlize::Network::apply(const account_name& code, const action_name& action) {
    auto &thiscontract = *this;

    if (action == N(transfer)) {
        auto transfer_data = unpack_action_data<st_transfer>();
        transfer(transfer_data.from, transfer_data.to, extended_asset(transfer_data.quantity, code), transfer_data.memo);
        return;
    }

    if (code != _self || action == N(transfer)) return;
    switch (action) {
        EOSIO_API(Dexlize::Network, (create)(cancel)(version)(start)(pause)(kill));
        default:
        eosio_assert(false, "Not contract action cannot be accepted");
        break;
    };
}


/**
 * function: user can create order of bought/selled by this function
 * parameter: token - should set the contract address of order e.g "elementscoin"
 *            memo - json format e.g. {"type": "1", "exed": "10000.0000 ELE", "ex": "10.0000 EOS", "contract": "elementscoin"}
 *                                    {"type": "2", "exed": "10000.0000 ELE", "ex": "10.0000 EOS", "contract": "elementscoin"}
 **/
void Dexlize::Network::create(const account_name& from, const string& memo) {
    require_auth(from);

    eosio_assert(is_running(), "game is already stop");
    // parse the memo
    map<string, string> memoMap;
    eosio_assert(parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    auto type_ptr = memoMap.find("type");
    eosio_assert(type_ptr != memoMap.end(), "must set type of bill in the memo");
    asset exchanged, exchange;
    account_name contract;
    _parseMemo(memoMap, exchanged, exchange, contract);

    // create current account
    tb_accounts accounts(_self, from);
    auto acc_ptr = accounts.find(contract);
    if (acc_ptr == accounts.end()) {
        accounts.emplace(from, [&](auto& a) {
            a.name = contract;
        });
    }

    // add contract
    auto contract_ptr = find(_global.contracts.begin(), _global.contracts.end(), contract);
    if (contract_ptr == _global.contracts.end()) {
        _global.contracts.emplace_back(contract);
    }

    // create order of selled/bought
    if (type_ptr->second == "1") {
        uint64_t buy_id = _next_buy_id();
        tb_buys buys(_self, contract);
        auto buy_ptr = buys.find(buy_id);
        if (buy_ptr == buys.end()) {
            buys.emplace(from, [&](auto& a) {
                a.id = buy_id;
                a.name = from;
                a.exchanged = extended_asset(exchanged, contract);
                a.exchange = extended_asset(exchange, N(eosio.token));
                a.amount = exchange.amount;
                a.actived = false;
            });
        }

        accounts.modify(accounts.find(contract), 0, [&](auto& a) {
            a.buys.emplace_back(buy_id);
        });
    } else {
        uint64_t sell_id = _next_sell_id();
        tb_sells sells(_self, contract);
        auto sell_ptr = sells.find(sell_id);
        if (sell_ptr == sells.end()) {
            sells.emplace(from, [&](auto& a) {
                a.id = sell_id;
                a.name = from;
                a.exchanged = extended_asset(exchanged, contract);
                a.exchange = extended_asset(exchange, N(eosio.token));
                a.amount = exchange.amount;
                a.actived = false;
            });
        }

        accounts.modify(accounts.find(contract), 0, [&](auto& a) {
            a.buys.emplace_back(sell_id);
        });
    }
}

/**
 * function: user can sell and buy token in the network of dexlize
 * paraneter: memo - json format e.g. {"type": "1", "id": "10001", "contract": "elementscoin"},
 *                                    {"type": "2", "id": "10001", "contract": "elementscoin"},
 *                                    {"type": "3", "id": "10001", "contract": "elementscoin"}, 
 *                                    {"type": "4", "id": "10001", "contract": "elementscoin"}
 * description: type: 1, the action is the order bought of user
 *              type: 2, the action is the order selled od user
 *              type: 3, the action is that user want to buy token by costing eos in the order selled
 *              type: 4, the action is that user want to get eos by costing token in the order bought
 **/
void Dexlize::Network::transfer(const account_name& from, const account_name& to, const extended_asset& quantity, const string& memo) {
    require_auth(from);

    eosio_assert(is_running(), "game is already stop");
    // check the account of from and to
    if (from == _self && to != _self) return;
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    // parse the memo of json fomat to get the transfer type
    map<string, string> memoMap;
    eosio_assert(parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    auto type_ptr = memoMap.find("type");
    eosio_assert(type_ptr != memoMap.end(), "must set type of bill in the memo");
    uint64_t type = stoi(type_ptr->second);
    eosio_assert(type > 0 && type < 5, "the format of type is not correct in the memo");
    uint64_t id;
    account_name contract;
    _parseMemo(memoMap, id, contract);
    
    switch (type)
    {
        case 1:
            _activeBuyOrder(quantity - fee, contract, id);
            break;
        case 2:
            _activeSellOrder(quantity - fee, contract, id);
            break;
        case 3:
            _buy(from, quantity - fee, contract, id);
            break;
        case 4:
            _sell(from, quantity - fee, contract, id);
            break;
        default:
            break;
    }
}

/**
 * function: user can cancel bill of sell and buy by bill id in the network of dexlize
 * paraneter: memo - json format e.g. {"type": "3"}
 *                                    {"type": "4"}
 **/
void Dexlize::Network::cancel(const account_name& from, const account_name& contract, uint64_t id, const string& memo) {
    require_auth(from);

    // check the memo and bill id
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    tb_accounts accounts(_self, from);
    auto iter = accounts.find(contract);
    eosio_assert(iter != accounts.end(), "the contract is not exist in the current account table");
    // parse the memo of json fomat to get the transfer type
    map<string, string> memoMap;
    eosio_assert(parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    auto type_ptr = memoMap.find("type");
    eosio_assert(type_ptr != memoMap.end(), "must set type of bill in the memo");
    if (type_ptr->second == "4") {
        // remove the selled bill id of current account
        auto bill_ptr = find(iter->sells.begin(), iter->sells.end(), id);
        eosio_assert(bill_ptr != iter->sells.end(), "the bill id is not exist in the sell bills of current account");
        accounts.modify(iter, 0, [&](auto& a) {
            a.sells.erase(bill_ptr);
        });

        // remove the selled 
        tb_sells sells(_self, contract);
        auto sell_ptr = sells.find(id);
        eosio_assert(sell_ptr != sells.end(), "the selled bill is not exist");
        if (sell_ptr->actived) {
            action(permission_level{_self, N(active)},
                sell_ptr->exchanged.contract,
                N(transfer),
                make_tuple(_self,
                            sell_ptr->name,
                            sell_ptr->exchanged.amount,
                            string("return the remaining token of order"))).send();
        }
        sells.erase(sell_ptr);
    } else if (type_ptr->second == "3") {
        // remove the bought bill id of current account
        auto bill_ptr = find(iter->buys.begin(), iter->buys.end(), id);
        eosio_assert(bill_ptr != iter->buys.end(), "the bill id is not exist in the buy bills of current account");
        accounts.modify(iter, 0, [&](auto& a) {
            a.buys.erase(bill_ptr);
        });

        // remove the bought bill
        tb_buys buys(_self, contract);
        auto buy_ptr = buys.find(id);
        eosio_assert(buy_ptr != buys.end(), "the bought bill is not exist");
        if (buy_ptr->actived) {
            action(permission_level{_self, N(active)},
                buy_ptr->exchanged.contract,
                N(transfer),
                make_tuple(_self,
                            buy_ptr->name,
                            buy_ptr->exchanged.amount,
                            string("return the remaining token of order"))).send();
        }
        buys.erase(buy_ptr);
    }
}

void Dexlize::Network::start() {
    require_auth(_self);

    eosio_assert(!is_running(), "game is running");
    set_game_status(true);
}

void Dexlize::Network::pause() {
    require_auth(_self);
    
    eosio_assert(is_running(), "game is already stop");
    set_game_status(false);
}

void Dexlize::Network::kill() {
    require_auth(_self);

    eosio_assert(!is_running(), "game is running");
    tb_sells sells(_self, contract);
    tb_buys buys(_self, contract);
    for (auto contract_ptr = _global.contracts.begin(); contract_ptr != _global.contracts.end(); ++contract_ptr) {
        tb_sells sells(_self, *contract_ptr);
        for (auto order_ptr = sells.begin(); order_ptr = sells.end(); ++order_ptr) {
            if (order_ptr->actived) {
                action(permission_level{_self, N(active)},
                    order_ptr->exchanged.contract,
                    N(transfer),
                    make_tuple(_self,
                                order_ptr->name,
                                asset(order_ptr->exchanged.amount, order_ptr->exchanged.symbol),
                                string("return the remaining token of order"))).send();
            }
        }
        for (auto order_ptr = sells.begin(); order_ptr = sells.end(); ++order_ptr) {
            sells.erase(order_ptr);
            // remove the selled bill id of current account
            auto bill_ptr = find(iter->sells.begin(), iter->sells.end(), order_ptr->id);
            eosio_assert(bill_ptr != iter->sells.end(), "the bill id is not exist in the sell bills of current account");
            accounts.modify(iter, 0, [&](auto& a) {
                a.sells.erase(bill_ptr);
            });
        }

        tb_buys buys(_self, *contract_ptr);
        for (auto order_ptr = buys.begin(); order_ptr = buys.end(); ++order_ptr) {
            if (order_ptr->actived) {
                action(permission_level{_self, N(active)},
                    order_ptr->exchange.contract,
                    N(transfer),
                    make_tuple(_self,
                                order_ptr->name,
                                asset(order_ptr->exchange.amount, order_ptr->exchange.symbol),
                                string("return the remaining token of order"))).send();
            }
        }
        for (auto order_ptr = buys.begin(); order_ptr = buys.end(); ++order_ptr) {
            buys.erase(order_ptr);
            // remove the bought bill id of current account
            auto bill_ptr = find(iter->buys.begin(), iter->buys.end(), order_ptr->id);
            eosio_assert(bill_ptr != iter->buys.end(), "the bill id is not exist in the buy bills of current account");
            accounts.modify(iter, 0, [&](auto& a) {
                a.buys.erase(bill_ptr);
            });
        }
    }
}