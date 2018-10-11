/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include "dexlize.hpp"
#include "utils.hpp"

symbol_name Dexlize::Proxy::_string_to_symbol_name(const string& str)
{
    return string_to_symbol(0, str.c_str()) >> 8;
}

symbol_name Dexlize::Proxy::_getSymbolName(const string& memo)
{
    Utils utils;
    map<string, string> memoMap = utils.parseJson(memo);

    auto iter = memoMap.find("symbol");
    if (iter != memoMap.end()) 
    {
        return _string_to_symbol_name(iter->second);
    }
    eosio_assert(iter == memoMap.end(), "invalid memo format for symbol");
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
    symbol_name symbol = _getSymbolName(memo);

    // get account of contract by the different symbol name in the memo
    // construct the memo of action by the different symbol name in the memo
    // and then execute the transfer action of target contract
    string actionMemo;
    account_name contract;
    switch (symbol) {
        case PUB_SYMBOL_NAME:
        contract = DAP_CONTRACT;
        actiontMemo = string("PUB-referrer:").append(GOD_ACCOUNT);
        break;
        case TPT_SYMBOL_NAME:
        contract = DAP_CONTRACT;
        actionMemo = string("TPT-referrer:").append(GOD_ACCOUNT);
        break;
        case KBY_SYMBOL_NAME:
        contract = KBY_CONTRACT;
        actionMemo = "";
        break;
    } 
    action(permission_level{_self, N(active)},
        contract, N(transfer),
        make_tuple(_self, from, quantity, actionMemo)
    ).send();
}

void Dexlize::Proxy::sell() 
{

}

void Dexlize::Proxy::convert() 
{

}

extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        Dexlize::Proxy thiscontract(receiver);

        if((code == N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &Dexlize::Proxy::buy);
            return;
        }

        if (code != receiver) return;

        switch (action) {
            EOSIO_API(Dexlize::Proxy, (version)(buy)(sell))
        };
        eosio_exit(0);
    }
}