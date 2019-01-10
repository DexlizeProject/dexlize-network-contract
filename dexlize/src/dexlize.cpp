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

bool Dexlize::Network::_parseMemo(const map<string, string>& memoMap, string& type, symbol_name& symbol, double& amount, account_name& contract) {
    bool reVal = false;

    auto type_ptr = memoMap.find("type");
    eosio_assert(type_ptr != memoMap.end(), "must set type of bill in the memo");
    type = type_ptr->second;
    eosio_assert(type == "sell" || type == "buy", "the bill type must be sell or buy");

    auto symbol_ptr = memoMap.find("symbol");
    eosio_assert(symbol_ptr != memoMap.end(), "must set the symbol in the memo");
    symbol = string_to_symbol(4, symbol_ptr->second.c_str());

    auto amount_ptr = memoMap.find("amount");
    eosio_assert(amount_ptr != memoMap.end(), "must set the amount in the memo");
    amount = stod(amount_ptr->second) * 10000;
    eosio_assert(amount > 1, "the order amount must greater than zero");

    auto contract_ptr = memoMap.find("contract");
    eosio_assert(contract_ptr != memoMap.end(), "must set the contract in the memo");
    contract = string_to_name(contract_ptr->second.c_str());

    return true;
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
 * function: user can sell and buy token in the network of dexlize
 * paraneter: memo - json format e.g. {"type": "sell_order", "amount": "10000.0000", "symbol": "EOS", "contract": "eosio.code"},
 *                                    {"type": "buy_order", "amount": "10000.0000", "symbol": "ELE", "contract": "elementscoin"}
 *                                    {"type": "sell", "amount": "10000.0000 ELE", "id": "10001"} 
*                                     {"type": "buy", "amount": "10000.0000 ELE", "id": "10001"}
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
    string type;
    symbol_name symbol;
    double amount;
    account_name contract;
    _parseMemo(memoMap, type, symbol, amount, contract);
    // add current account
    auto acc_ptr = _accounts.find(from);
    if (acc_ptr == _accounts.end()) {
        _accounts.emplace(from, [&](auto& a) {
            a.name = from;
        });
    }
    eosio_assert(acc_ptr != _accounts.end(), "the account is not exist in the accounts");
    // add the bill of sell or buy into table
    if (type == "sell") {
        uint64_t sell_id = _next_sell_id();
        _sells.emplace(from, [&](auto& a) {
            a.id = sell_id;
            a.name = from;
            a.quantity = quantity;
            a.amount = extended_asset(asset(amount, symbol), contract);
        });

        acc_ptr = _accounts.find(from);
        _accounts.modify(acc_ptr, 0, [&](auto& a) {
            a.sells.emplace_back(sell_id);
        });
    } else if (type == "buy") {
        eosio_assert(quantity.contract == N(eosio.token), "must pay with EOS token by eosio.token");
        eosio_assert(quantity.symbol == EOS_SYMBOL, "must pay with EOS token");
        uint64_t buy_id = _next_buy_id();
        _buys.emplace(from, [&](auto& a) {
            a.id = buy_id;
            a.name = from;
            a.quantity = extended_asset(asset(amount, symbol), contract);
            a.amount = quantity;
        });

        acc_ptr = _accounts.find(from);
        _accounts.modify(acc_ptr, 0, [&](auto& a) {
            a.buys.emplace_back(buy_id);
        });
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