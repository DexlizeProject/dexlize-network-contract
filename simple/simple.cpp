#include "simple.hpp"

void Simple::simple::version()
{

}

void Simple::simple::transfer(account_name from, account_name to, asset quantity, string memo)
{

}

void Simple::simple::create(account_name issuer, asset maximum_supply)
{
    require_auth(_self);

    auto symbol = maximum_supply.symbol;
    eosio_assert(symbol.is_valid(), "invalid symbol name");
    eosio_assert(maximum_supply.is_valid(), "invalid supply");
    eosio_assert(maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable(_self, symbol.name());
    auto existing = statstable.find(symbol.name());
    eosio_assert(existing == statstable.end(), "token with symbol already exists");

    statstable.emplace(_self, [&]( auto& s) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply    = maximum_supply;
        s.issuer        = issuer;
    });
}

void Simple::simple::issue(account_name to, asset quantity, string memo)
{

}

void Simple::simple::newtoken()
{

}

extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        Simple::simple thiscontract(receiver);

        if((code == N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &Simple::simple::buy);
            return;
        }

        if (code != receiver) return;

        switch (action) {
            EOSIO_API(Simple::simple, (version)(transfer)(sell)(newtoken))
        };
        eosio_exit(0);
    }
}
