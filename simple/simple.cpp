#include "simple.hpp"

void Simple::simple::version()
{

}

void Simple::simple::transfer(account_name from, account_name to, asset quantity, string memo)
{
    print("transfer from ", eosio::name{from}, " to ", eosio::name{to}, " ", quantity, "\n");
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
    auto symbol = quantity.symbol;
    eosio_assert(symbol.is_valid(), "invalid symbol name");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    auto symbol_name = symbol.name();
    stats statstable(_self, symbol_name);
    auto existing = statstable.find(symbol_name);
    eosio_assert(existing != statstable.end(), "token with symbol does not exist, create token before issue");
    const auto& st = *existing;

    require_auth(st.issuer);
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");

    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });
}

void Simple::simple::newtoken(account_name from, asset maximum_stake)
{
    SEND_INLINE_ACTION(*this, create, {from, N(active)}, {from, maximum_stake});
    SEND_INLINE_ACTION(*this, issue, {from, N(active)}, {from, maximum_stake, string("")});
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
