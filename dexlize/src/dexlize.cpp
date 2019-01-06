/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include "../include/dexlize.hpp"
#include "../utils/utils.hpp"

std::string Dexlize::Aux::_getMemoValue(const string& key, const map<string, string>& memoMap) const
{
    auto iter = memoMap.find(key);
    if (iter != memoMap.end()) 
    {
        return iter->second;
    }
    eosio_assert(iter == memoMap.end(), "invalid memo format");
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
 * paraneter: memo - json format e.g. {"onwer": "eosbitportal"}
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

void Dexlize::Network::transfer(const account_name& from, const account_name& to, const extended_asset& quantity, const string& memo) {
    require_auth(from);

    // check the account of from and to
    if (from == _self && to != _self) return;
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
}

void Dexlize::Network::cancel(const account_name& from, const uint64_t& bill_id, const string& memo) {
    require_auth(from);

    // check the memo and bill id
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    auto iter = _accounts.find(from);
    eosio_assert(iter != _accounts.end(), "current account is not exist");
    eosio_assert(memo == "sell" || memo == "buy", "the format of memo is not correct");
    if (memo == "sell") {
        eosio_assert(find(iter->sells.begin(), iter->sells.end(), [](auto a) {return a == bill_id;})
        != iter->sells.end(), "the bill id is not exist in the sell bills");
    } else (memo == "buy") {
        eosio_assert(find(iter->buys.begin(), iter->buys.end(), [](auto a) {return a == bill_id;})
        != iter->buys.end(), "the bill id is not exist in the buy bills");
    }
}