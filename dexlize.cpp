/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include "dexlize.hpp"
#include "utils.hpp"

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

account_name Dexlize::Aux::getContractAccountName(symbol_name symbolName)
{
    account_name contractName;
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
    return contractName;
}

string Dexlize::Aux::getActionMemo(symbol_name symbolName)
{
    string memo;
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
    return memo;
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
    if (from == _self || to != _self) return;
    eosio_assert(quantity.symbol == CORE_SYMBOL, "must pay with CORE token");

    // take the symbol name, and the check if the symbol name of PUB/TPT is in the memo
    Utils utils;
    map<string, string> memoMap = utils.parseJson(memo);
    symbol_name symbolName = aux.getSymbolName(memoMap);

    // get account of contract by the different symbol name in the memo
    // construct the memo of action by the different symbol name in the memo
    // and then execute the transfer action of target contract
    account_name contract = aux.getContractAccountName(symbolName);
    string actionMemo = aux.getActionMemo(symbolName);
    action(permission_level{_self, N(active)},
        N(eosio.token), N(transfer),
        make_tuple(_self, contract, quantity, actionMemo)
    ).send();

    // get the stake quantity and then execute transfer action
    asset stake = aux.getStakeAmount(memoMap);
    action(permission_level{_self, N(active)}, 
        contract, N(transfer),
        make_tuple(_self, from, stake, string("dexlize transfer https://dexlize.io"))
    ).send();
}

void Dexlize::Proxy::sell(account_name from, account_name to, asset quantity, string memo)
{
    require_auth(from);
    // check the account of from and to
    
    // execute the sell action of tokendapppub, transfer quantity from the account of from to dexlize
    account_name contract = aux.getContractAccountName(quantity.symbol.name);
    action(permission_level{_self, N(active)},
        contract,
        N(sell),
        make_tuple(_self, contract, quantity, string("dexlize withdraw https://dexlize.io"))
    ).send();

    // execute the transfer action of eosio.token, transfer the eos from dexlize to the account of from
    Utils utils;
    map<string, string> memoMap = utils.parseJson(memo);
    asset eos = aux.getEosAmount(memoMap);
    action(permission_level{_self, N(active)},
        N(eosio.token),
        N(transfer),
        make_tuple(_self, from, eos, string("dexlize withdraw https://dexlize.io"))
    ).send();
}

void Dexlize::Proxy::convert(account_name from, asset fromAsset, asset toAsset) 
{
    require_auth(from);

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
            EOSIO_API(Dexlize::Proxy, (version)(sell))
        };
        eosio_exit(0);
    }
}