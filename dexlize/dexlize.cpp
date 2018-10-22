/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include "dexlize.hpp"
#include "utils/utils.hpp"

std::string Dexlize::Aux::_getMemoValue(const string& key, const map<string, string>& memoMap)
{
    auto iter = memoMap.find(key);
    if (iter != memoMap.end()) 
    {
        return iter->second;
    }
    eosio_assert(iter == memoMap.end(), "invalid memo format");
}

eosio::symbol_name Dexlize::Aux::getSymbolName(const map<string, string>& memoMap)
{
    return SN(_getMemoValue("symbol", memoMap));
}

std::string Dexlize::Aux::getOwnerAccount(const map<string, string>& memoMap)
{
    return _getMemoValue("owner", memoMap);
}

account_name Dexlize::Aux::getContractAccount(const map<string, string>& memoMap)
{
    return N(_getMemoValue("contract", memoMap));
}

std::string Dexlize::Aux::getActionMemo(symbol_name symbolName, string owner)
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

void Dexlize::Proxy::_sendAction(account_name contractAccount, account_name to, asset quantity, string actionStr, string memo)
{
    action(permission_level{_self, N(active)},
        contractAccount,
        N(actionStr),
        make_tuple(_self, to, quantity, memo)
    ).send();
}

bool Dexlize::Proxy::_checkSymbol(account_name contractAccount, symbol_name symbolName)
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
void Dexlize::Proxy::version()
{

}

/**
 * function: user can transfer the eos to buy take by this interface, and also can set the stake beneficiary
 * parameter: memo - json format e.g. {"contract": "dexlize", "symbol": "DEX", "owner": "eosbitportal"}
 * */
void Dexlize::Proxy::buy(account_name from, account_name to, asset eos, string memo)
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
void Dexlize::Proxy::sell(account_name from, account_name to, asset quantity, string memo)
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

extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        Dexlize::Proxy thiscontract(receiver);

        if((code == N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &Dexlize::Proxy::buy);
            return;
        } else if ((code != N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &Dexlize::Proxy::sell);
            return;
        }

        if (code != receiver) return;

        switch (action) {
            EOSIO_API(Dexlize::Proxy, (version))
        };
        eosio_exit(0);
    }
}