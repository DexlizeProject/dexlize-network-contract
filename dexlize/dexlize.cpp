/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include "dexlize.hpp"
#include "utils/utils.hpp"

symbol_name Dexlize::Aux::string2SymbolName(const string& str)
{
    return string_to_symbol(0, str.c_str()) >> 8;
}

string Dexlize::Aux::_getMemoValue(const string& key, const map<string, string>& memoMap)
{
    auto iter = memoMap.find(key);
    if (iter != memoMap.end()) 
    {
        return iter->second;
    }
    eosio_assert(iter == memoMap.end(), "invalid memo format");
}

symbol_name Dexlize::Aux::getSymbolName(const map<string, string>& memoMap)
{
    return string2SymbolName(_getMemoValue("symbol", memoMap));
}

asset Dexlize::Aux::getStakeAmount(const map<string, string>& memoMap)
{
    return asset(S(4, _getMemoValue("amount", memoMap)), PUB_SYMBOL);
}

asset Dexlize::Aux::getEosAmount(const map<string, string>& memoMap)
{
    return asset(S(4, _getMemoValue("eos", memoMap)), PUB_SYMBOL);
}

string Dexlize::Aux::getTransactionType(const map<string, string>& memoMap)
{
    return _getMemoValue("type", memoMap);
}

account_name Dexlize::Aux::getContractAccountName(symbol_name symbolName)
{
    account_name contractName = null;
    switch (symbolName) 
    {
        case PUB_SYMBOL_NAME:
        contractName = DAP_CONTRACT;
        break;
        case TPT_SYMBOL_NAME:
        contractName = DAP_CONTRACT;
        break;
        // TODO: support the kyubey on the stage II
        case KBY_SYMBOL_NAME:
        contractName = KBY_CONTRACT;
        break;
    }
    eosio_assert(contractName != null, "token not found by this symbol name");
    return contractName;
}

string Dexlize::Aux::getActionMemo(symbol_name symbolName)
{
    string memo = null;
    switch (symbolName) 
    {
        case PUB_SYMBOL_NAME:
        memo = string("PUB-referrer:").append(GOD_ACCOUNT);
        break;
        case TPT_SYMBOL_NAME:
        memo = string("TPT-referrer:").append(GOD_ACCOUNT);
        break;
        // TODO: support the kyubey on the stage II
        case KBY_SYMBOL_NAME:
        memo = "";
        break;
    }
    eosio_assert(memo != null, "token not found by this symbol name");
    return memo;
}

void Dexlize::Proxy::_sendAction(account_name contract, account_name to, asset quantity, string actionStr, string memo)
{
    action(permission_level{_self, N(active)},
        contract,
        N(actionStr),
        make_tuple(_self, to, quantity, memo == "" ? "dexlize " + actionStr + " https://dexlize.org" : memo)
    ).send();
}

/**
 * function: 
 * parameter: 
 */
void Dexlize::Proxy::version()
{

}

void Dexlize::Proxy::buy(account_name from, account_name to, asset quantity, string memo)
{
    // check if the account name is correct.
    // from account cannot be owner and to account must be owner.
    if (from == _self || to != _self || from == DAP_CONTRACT) return;
    eosio_assert(quantity.symbol == CORE_SYMBOL, "must pay with CORE token");

    // take the symbol name, and the check if the symbol name of PUB/TPT is in the memo
    map<string, string> memoMap;
    Utils utils;
    if (!utils.parseJson(memo)) {
        eosio_assert(iter == memoMap.end(), "invalid memo format, the memo must be the format of json");
    }
    symbol_name symbolName = aux.getSymbolName(memoMap);

    // get account of contract by the different symbol name in the memo
    // construct the memo of action by the different symbol name in the memo
    // and then execute the transfer action of target contract
    account_name contract = aux.getContractAccountName(symbolName);
    string actionMemo = aux.getActionMemo(symbolName);
    _sendAction(EOSIO_CONTRACT, contract, quantity, ACTION_TRANSFER_TYPE, actionMemo);

    // get the stake quantity and then execute transfer action
    asset stake = aux.getStakeAmount(memoMap);
    _sendAction(contract, from, stake, ACTION_TRANSFER_TYPE);
}

void Dexlize::Proxy::sell(account_name from, account_name to, asset quantity, string memo)
{
    require_auth(from);
    // check the account of from and to
    if (from == _self || from == DAP_CONTRACT) return;

    // parse the memo of json fomat to get the transfer type
    map<string, string> memoMap;
    Utils utils;
    if (!utils.parseJson(memo, memoMap)) {
        eosio_assert(iter == memoMap.end(), "invalid memo format, the memo must be the format of json");
    }

    // get the type of transfer from json of memo parsed
    // the type of action in the memo is only way to define the mean of transfer user
    string type = aux.getTransactionType(memoMap);
    if (type == ACTION_SELL_TYPE) 
    {
        // execute the sell action of tokendapppub, transfer quantity from the account of from to dexlize
        account_name contract = aux.getContractAccountName(quantity.symbol.name);
        _sendAction(contract, contract, quantity, ACTION_SELL_TYPE);

        // execute the transfer action of eosio.token, transfer the eos from dexlize to the account of from
        asset eos = aux.getEosAmount(memoMap);
        _sendAction(EOSIO_CONTRACT, from, eos, ACTION_TRANSFER_TYPE);
    } 
    else if (type == ACTION_CONVERT_TYPE)
    {
        // sell the token of fromer account by symbol to account of contranct(e.g. tokendapppub)
        account_name contract = aux.getContractAccountName(quantity.symbol.name);
        _sendAction(contract, contract, quantity, ACTION_SELL_TYPE);

        // buy the token of to account by symbol from account of contract(e.g. tokendapppub)
        contract = aux.getContractAccountName(aux.getSymbolName(memoMap));
        _sendAction(EOSIO_CONTRACT, contract, quantity, ACTION_TRANSFER_TYPE);

        // transfer the brought token to user(fromer account)
        asset stake;
        _sendAction(contract, from, stake, ACTION_TRANSFER_TYPE);
    }
    else if (type == ACTION_TRANSFER_TYPE) 
    {
        // execute the sell action of tokendapppub, transfer quantity from the account of from to dexlize
        account_name contract = aux.getContractAccountName(quantity.symbol.name);
        _sendAction(contract, to, quantity, ACTION_TRANSFER_TYPE);
    }
}

extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        Dexlize::Proxy thiscontract(receiver);

        if((code == EOSIO_CONTRACT) && (action == N(transfer))) {
            execute_action(&thiscontract, &Dexlize::Proxy::buy);
            return;
        } else if ((code == DAP_CONTRACT) && (action == N(transfer))) {
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