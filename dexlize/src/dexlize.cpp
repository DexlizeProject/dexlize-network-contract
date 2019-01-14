/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include <algorithm>
#include "../include/dexlize.hpp"
#include "../utils/utils.hpp"

std::string Dexlize::Aux::_getMemoValue(const string& key, const map<string, string>& memoMap) const
{
    auto iter = memoMap.find(key);
    eosio_assert(iter != memoMap.end(), "invalid memo format");
    return iter->second;
}

eosio::symbol_name Dexlize::Aux::getSymbolName(const map<string, string>& memoMap) const
{
    return SN(_getMemoValue("symbol", memoMap));
}

std::string Dexlize::Aux::getOwnerAccount(const map<string, string>& memoMap) const
{
    return _getMemoValue("owner", memoMap);
}

account_name Dexlize::Aux::getContractAccount(const map<string, string>& memoMap) const
{
    return N(_getMemoValue("contract", memoMap));
}

std::string Dexlize::Aux::getActionMemo(const symbol_name& symbolName, const string& owner) const
{
    string memo = "dexlize";

    switch (symbolName)
    {
        case SN("PUB"):
        memo = string("PUB-referrer:").append(GOD_ACCOUNT);
        break;
        case SN("TPT"):
        memo = string("TPT-referrer:").append(GOD_ACCOUNT);
        break;
    }
    memo += "-owner:" + owner;

    return memo;
}

void Dexlize::Network::_sendAction(account_name contractAccount, account_name to, asset quantity, string actionStr, string memo)
{
    action(permission_level{_self, N(active)},
        contractAccount,
        N(actionStr),
        make_tuple(_self, to, quantity, memo)
    ).send();
}

bool Dexlize::Network::_checkSymbol(account_name contractAccount, symbol_name symbolName)
{
    bool reVal = false;

    // check if the contact account is supported
    auto iter = mapContract2Symbol.find(contractAccount);
    if (iter != mapContract2Symbol.end()) {
        // check if the symbol is supported
        for (auto value : iter->second) {
            if (value == symbolName) {
                reVal = true;
                break;
            }
        }
    }

    return reVal;
}

bool Dexlize::Network::_parseMemo(const map<string, string>& memoMap, uint64_t& id) {
    bool reVal = false;

    auto id_ptr = memoMap.find("id");
    eosio_assert(type_ptr != memoMap.end(), "must set order id in the memo when buying or selling");
    id = stoi(id_ptr->second);

    return true;
}

bool Dexlize::Network::_parseMemo(const map<string, string>& memoMap, symbol_name& symbol, int64_t& amount, account_name& contract) {
    bool reVal = false;

    auto symbol_ptr = memoMap.find("symbol");
    eosio_assert(symbol_ptr != memoMap.end(), "must set the symbol in the memo");
    symbol = string_to_symbol(4, symbol_ptr->second.c_str());

    auto amount_ptr = memoMap.find("amount");
    eosio_assert(amount_ptr != memoMap.end(), "must set the amount in the memo");
    amount = static_cast<int64_t>(stod(amount_ptr->second) * 10000);
    eosio_assert(amount > 1, "the order amount must greater than zero");

    auto contract_ptr = memoMap.find("contract");
    eosio_assert(contract_ptr != memoMap.end(), "must set the contract in the memo");
    contract = string_to_name(contract_ptr->second.c_str());

    return true;
}

void Dexlize::Network::_sell(uint64_t id, const account_name& from, const extended_asset& quantity) {
    auto sell_ptr = _sells.find(id);
    eosio_assert(sell_ptr != _sells.end(), "the sell order is not exist in the sells table");
    eosio_assert(sell_ptr->exchange >= quantity, "the quantity brought must be less than amount of order");
    eosio_assert(quantity.symbol == sell_ptr->exchange.symbol, "must pay with exchange token when buying token");
    uint64_t buy_amount(static_cast<double>(sell_ptr->exchange.amount) / static_cast<double>(quanity.amount) * sell_ptr->exchanged.amount);
    if (sell_ptr->exchange == quanity) {
        _sells.erase(sell_ptr);
        auto acc_ptr = _accounts.find(sell_ptr->name);
        _accounts.modify(acc_ptr, [&](auto& a) {
            auto acc_sell_ptr = find(a.sells.end(), a.sells.end(), id);
            eosio_assert(acc_sell_ptr != a.sells.end(), "sell id is not exist in the account table");
            a.sells.erase(acc_sell_ptr);
        });
    } else {
        _sells.modify(sell_ptr, [&](auto& a) {
            a.exchange -= quantity;
            a.exchanged.amount -= buy_amount;
        });
    }

    action(permission_level{_self, N(active)},
           sell_ptr->exchanged.contract,
           N(transfer),
           make_tuple(_self, from, asset(buy_amount, sell->exchanged.symbol), string("buy token, exchange: dexlize network"))).send();
}

void Dexlize::Network::_buy(uint64_t id, const account_name& from, const extended_asset& quantity) {
    auto buy_ptr = _buys.find(id);
    eosio_assert(buy_ptr != _buys.end(), "the buy order is not exist in the buys table");
    eosio_assert(buy_ptr->exchange >= quantity, "the quantity selled must be less than amount of order");
    eosio_assert(quantity.symbol == buy_ptr->exchange.symbol, "must pay with exchange token when selling token");
    uint64_t sell_amount(static_cast<double>(buy_ptr->exchange.amount) / static_cast<double>(quanity.amount) * buy_ptr->exchanged.amount);
    if (buy_ptr->exchange == quanity) {
        _sells.erase(buy_ptr);
        auto acc_ptr = _accounts.find(buy_ptr->name);
        _accounts.modify(acc_ptr, [&](auto& a) {
            auto acc_buy_ptr = find(a.buys.end(), a.buys.end(), id);
            eosio_assert(acc_buy_ptr != a.buys.end(), "sell id is not exist in the account table");
            a.buys.erase(acc_buy_ptr);
        });
    } else {
        _sells.modify(sell_ptr, [&](auto& a) {
            a.exchange -= quantity;
            a.exchanged.amount -= sell_amount;
        });
    }

    action(permission_level{_self, N(active)},
           buy_ptr->exchanged.contract,
           N(transfer),
           make_tuple(_self, from, asset(buy_amount, sell->exchanged.symbol), string("buy token, exchange: dexlize network"))).send();
}

void Dexlize::Network::_sellOrder(const account_name& from, const extended_asset& quantity, const account_name& contract, symbol_name symbol, int64_t amount) {
    uint64_t sell_id = _next_sell_id();
    tb_sells sells(_self, quantity.contract);
    auto sell_ptr = sells.find(sell_id);
    _sells.emplace(from, [&](auto& a) {
        a.id = sell_id;
        a.name = from;
        a.exchanged = quantity;
        a.exchange = extended_asset(asset(amount, symbol), contract);
        a.mount = amount;
    });

    acc_ptr = _accounts.find(from);
    _accounts.modify(acc_ptr, 0, [&](auto& a) {
        a.sells.emplace_back(sell_id);
    });
}

void Dexlize::Network::_buyOrder(const account_name& from, const extended_asset& quantity, const account_name& contract, const symbol_name& symbol, int64_t amount) {
    eosio_assert(quantity.contract == N(eosio.token), "must pay with EOS token by eosio.token");
    eosio_assert(quantity.symbol == EOS_SYMBOL, "must pay with EOS token");
    uint64_t buy_id = _next_buy_id();
    _buys.emplace(from, [&](auto& a) {
        a.id = buy_id;
        a.name = from;
        a.exchanged = extended_asset(asset(amount, symbol), contract);
        a.exchange = quantity;
        a.amount = amount; 
    });

    acc_ptr = _accounts.find(from);
    _accounts.modify(acc_ptr, 0, [&](auto& a) {
        a.buys.emplace_back(buy_id);
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
    // check if the account name is correct.
    // from account cannot be owner and to account must be owner.
    if (from == _self || to != _self) return;
    eosio_assert(eos.symbol == CORE_SYMBOL, "must pay with CORE token");

    // take the symbol name, and the check if the symbol name is in the memo
    Utils utils;
    map<string, string> memoMap;
    eosio_assert(!utils.parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    account_name contractAccount = aux.getContractAccount(memoMap);
    symbol_name symbolName = aux.getSymbolName(memoMap);
    eosio_assert(_checkSymbol(contractAccount, symbolName), "current symbol is not supported");

    // get account of contract by the different symbol name in the memo
    // construct the memo of action by the different symbol name in the memo
    // and then execute the transfer action of target contract
    string owner = aux.getOwnerAccount(memoMap);
    string actionMemo = aux.getActionMemo(symbolName, owner);
    _sendAction(N(eosio.token), contractAccount, eos, ACTION_TRANSFER_TYPE, actionMemo);
}

/**
 * function: user can sell stake to gain the eos, and also can set the selled stake beneficiary
 * paraneter: memo - json format e.g. {"owner": "eosbitportal"}
 **/
void Dexlize::Network::sell(account_name from, account_name to, asset quantity, string memo)
{
    require_auth(from);

    // check the account of from and to
    if (from == _self && to != _self) return;
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    // parse the memo of json fomat to get the transfer type
    Utils utils;
    map<string, string> memoMap;
    eosio_assert(!utils.parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    account_name contractAccount = aux.getContractAccount(memoMap);
    symbol_name symbolName = aux.getSymbolName(memoMap);
    eosio_assert(_checkSymbol(contractAccount, symbolName), "current symbol is not supported");

    // get the contract account from the asset symbol
    // execute the action of sell by the target contract
    string owner = aux.getOwnerAccount(memoMap); 
    _sendAction(contractAccount, contractAccount, quantity, ACTION_SELL_TYPE, "-owner:" + owner);
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
        EOSIO_API(Dexlize::Network, (cancel)(version));
        default:
        eosio_assert(false, "Not contract action cannot be accepted");
        break;
    };
}


/**
 * function: user can create order of bought/selled by this function
 * parameter: token - should set the contract address of order e.g "elementscoin"
 *            memo - e.g "buy" or "sell"
 **/
void Dexlize::Network::create(const account_name& from, const account_name& token, const string& memo) {
    require_auth(from);

    eosio_assert(memo == "buy" || memo == "sell", "must set the type of order in the memo");

    // create current account
    tb_accounts accounts(_self, from);
    auto acc_ptr = accounts.find(token);
    if (acc_ptr == accounts.end()) {
        accounts.emplace(from, [&](auto& a) {
            a.name = token;
        });
    }

    // create order of selled/bought
    if (memo == "buy") {
        uint64_t buy_id = _next_buy_id();
        tb_buys buys(_self, token);
        auto buy_ptr = buys.find(buy_id);
        if (buy_ptr == buys.end()) {
            buys.emplace((from, [&](auto& a) {
                a.id = buy_id;
                a.name = from;
                a.exchanged = extended_asset(sset(0, EOS_SYMBOL), N(eosio.token));
                a.exchange = extended_asset(asset(0, EOS_SYMBOL), N(eosio.token));
                a.mount = 0;
                a.actived = false;
            });
        }

        accounts.modify(accounts.find(token), [&](auto& a) {
            a.buys.emplace_back(buy_id);
        });
    } else {
        uint64_t sell_id = _next_sell_id();
        tb_sells sells(_self, token);
        auto sell_ptr = sells.find(sell_id);
        if (sell_ptr == sells.end()) {
            sells.emplace((from, [&](auto& a) {
                a.id = sell_id;
                a.name = from;
                a.exchanged = extended_asset(sset(0, EOS_SYMBOL), N(eosio.token));
                a.exchange = extended_asset(asset(0, EOS_SYMBOL), N(eosio.token));
                a.mount = 0;
                a.actived = false;
            });
        }

        accounts.modify(accounts.find(token), [&](auto& a) {
            a.buys.emplace_back(sell_id);
        });
    }
}

/**
 * function: user can sell and buy token in the network of dexlize
 * paraneter: memo - json format e.g. {"type": "1", "amount": "10000.0000", "symbol": "EOS", "contract": "eosio.code"},
 *                                    {"type": "2", "amount": "10000.0000", "symbol": "ELE", "contract": "elementscoin"}
 *                                    {"type": "3", "id": "10001"} 
 *                                    {"type": "4", "id": "10001"}
 * description: type: 1, the action is the order bought of user
 *              type: 2, the action is the order selled od user
 *              type: 3, the action is that user want to buy token by costing eos in the order selled
 *              type: 4, the action is that user want to get eos by costing token in the order bought
 **/
void Dexlize::Network::transfer(const account_name& from, const account_name& to, const extended_asset& quantity, const string& memo) {
    require_auth(from);

    // check the account of from and to
    if (from == _self && to != _self) return;
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    // parse the memo of json fomat to get the transfer type
    Utils utils;
    map<string, string> memoMap;
    eosio_assert(!utils.parseJson(memo, memoMap), "invalid memo format, the memo must be the format of json");
    auto type_ptr = memoMap.find("type");
    eosio_assert(type_ptr != memoMap.end(), "must set type of bill in the memo");

    uint64_t type = stoi(type_ptr->second);
    if (type == 1 || type == 2) {
        symbol_name symbol;
        int64_t amount;
        account_name contract;
        _parseMemo(memoMap, symbol, amount, contract);
        // add the order of sell or buy into table
        if (type == 1) {
            _buyOrder(from, quantity, contract, symbol, amount);
        } else {
            _sellOrder(from, quantity, contract, symbol, amount);
        }
    } else if (type == 3 || type == 4) {
        uint64_t id;
        _parseMemo(memoMap, id);
        // buy or sell token
        if (type == 3) {
            _buy(id, from, quantity);
        } else {
            _sell(id, from, quantity);
        }
    }
}

/**
 * function: user can cancel bill of sell and buy by bill id in the network of dexlize
 * paraneter: memo - json format e.g. {sell/buy}
 **/
void Dexlize::Network::cancel(const account_name& from, const uint64_t& bill_id, const string& memo) {
    require_auth(from);

    // check the memo and bill id
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    auto iter = _accounts.find(from);
    eosio_assert(iter != _accounts.end(), "current account is not exist");
    eosio_assert(memo == "sell" || memo == "buy", "the format of memo is not correct");
    if (memo == "sell") {
        // remove the selled bill id of current account
        auto bill_ptr = find(iter->sells.begin(), iter->sells.end(), bill_id);
        eosio_assert(bill_ptr != iter->sells.end(), "the bill id is not exist in the sell bills of current account");
        _accounts.modify(iter, 0, [&](auto& a) {
            a.sells.erase(bill_ptr);
        });

        // remove the selled bill
        auto sell_ptr = _sells.find(bill_id);
        eosio_assert(sell_ptr != _sells.end(), "the selled bill is not exist");
        _sells.erase(sell_ptr);
    } else if (memo == "buy") {
        // remove the bought bill id of current account
        auto bill_ptr = find(iter->buys.begin(), iter->buys.end(), bill_id);
        eosio_assert(bill_ptr != iter->buys.end(), "the bill id is not exist in the buy bills of current account");
        _accounts.modify(iter, 0, [&](auto& a) {
            a.buys.erase(bill_ptr);
        });

        // remove the bought bill
        auto buy_ptr = _buys.find(bill_id);
        eosio_assert(buy_ptr != _buys.end(), "the bought bill is not exist");
        _buys.erase(buy_ptr);
    }
}