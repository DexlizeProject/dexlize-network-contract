/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include "dexlize.hpp"

symbol_name Dexlize::Proxy::_string_to_symbol_name(const char* str)
{
    return string_to_symbol(0, str) >> 8;
}

map<string, string> Dexlize::Proxy::_parseMemo(string memo)
{
    map<string, string> memoMap;
    return memoMap;
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
    if (from == _self || to != _self)
    {
        return;
    }
    eosio_assert(quantity.symbol == CORE_SYMBOL, "must pay with CORE token");

    // check if the symbol name of PUB/TPT is in the memo
    if (memo.find("-symbol") != string::npos)
    {
        auto first_separator_pos = memo.find("-symbol");
        eosio_assert(first_separator_pos != string::npos, "invalid memo format for symbol");

        string name_str = memo.substr(0, first_separator_pos);
    }
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