#include "simple.hpp"

void dexlize::simple::_sub_balance(account_name owner, asset value) {
    accounts from_acnts(_self, owner);

    const auto& from = from_acnts.get(value.symbol.name(), "no balance object found by from account");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

    if(from.balance.amount == value.amount) {
        from_acnts.erase(from);
    } else {
        from_acnts.modify(from, owner, [&]( auto& a) {
            a.balance -= value;
        });
    }
}

void dexlize::simple::_add_balance(account_name owner, asset value, account_name ram_payer)
{
    accounts to_acnts(_self, owner);
    auto to = to_acnts.find(value.symbol.name());
    if(to == to_acnts.end()) {
        to_acnts.emplace(ram_payer, [&]( auto& a){
            a.balance = value;
        });
    } else {
        to_acnts.modify(to, 0, [&]( auto& a) {
            a.balance += value;
        });
    }
}

void dexlize::simple::version()
{

}

void dexlize::simple::transfer(account_name from, account_name to, asset quantity, string memo)
{
    require_auth(from);
    eosio_assert(is_account(to), "to account does not exist");
    eosio_assert(from != to, "cannot transfer to self");
    auto symbol_name = quantity.symbol.name();

    tb_simples simple_sgt(_self, symbol_name);
    eosio_assert(simple_sgt.exists(), "simple not found by this symbol name");
    st_simple simple = simple_sgt.get();

    require_recipient(from);
    require_recipient(to);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol == simple.symbol, "symbol precision mismatch");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    _sub_balance(from, quantity);
    _add_balance(to, quantity, from);
}

void dexlize::simple::buy(account_name from, account_name to, asset quantity, string memo)
{
    if (from == _self || to != _self) return;
    eosio_assert(quantity.symbol == CORE_SYMBOL, "must pay with CORE token");

    auto iter = memo.find("-owner:");
    account_name owner;
    if (iter != string::npos) {
        string owner_string = memo.substr(memo.find(":", iter) + 1);
    }
}

void dexlize::simple::sell(account_name from, asset quantity, string memo) 
{

}

void dexlize::simple::create(account_name issuer, asset maximum_supply)
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

void dexlize::simple::issue(account_name to, asset quantity, string memo)
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

void dexlize::simple::newsimple(account_name from, asset base_eos_quantity, asset maximum_stake, 
        uint32_t lock_up_period, uint8_t base_fee_percent, uint8_t init_fee_percent, uint32_t start_time)
{
    require_auth(from);
    _init_simple(from, base_eos_quantity, maximum_stake, lock_up_period, base_fee_percent, init_fee_percent, start_time);
    
    SEND_INLINE_ACTION(*this, create, {from, N(active)}, {from, maximum_stake});
    SEND_INLINE_ACTION(*this, issue, {from, N(active)}, {from, maximum_stake, string("")});
}

extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        dexlize::simple thiscontract(receiver);

        if((code == N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &dexlize::simple::buy);
            return;
        }

        if (code != receiver) return;

        switch (action) {
            EOSIO_API(dexlize::simple, (version)(transfer)(sell)(newsimple))
        };
        eosio_exit(0);
    }
}
