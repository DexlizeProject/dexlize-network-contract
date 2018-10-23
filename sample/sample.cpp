#include "sample.hpp"

void dexlize::sample::_sub_balance(account_name owner, asset value) {
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

void dexlize::sample::_add_balance(account_name owner, asset value, account_name ram_payer)
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

std::pair<eosio::asset, eosio::asset> dexlize::sample::_sample_sell(symbol_name name, int64_t stake)
{
    tb_samples sample_sgt(_self, name);
    eosio_assert(sample_sgt.exists(), "game not found by this symbol name");

    st_sample sample = sample_sgt.get();
    eosio_assert(now() > sample.start_time, "the token issuance has not yet begun");

    asset eos = asset(sample.sell(stake), CORE_SYMBOL);
    asset fee = asset(sample.fee(eos.amount), CORE_SYMBOL);

    eosio_assert(eos.amount > 0, "eos amount should be bigger than 0");
    eosio_assert(eos.amount < sample.eos - sample.base_eos, "eos amount overflow");

    sample_sgt.set(sample, sample.owner);

    return make_pair(eos, fee);
}

eosio::asset dexlize::sample::_sample_buy(symbol_name name, int64_t eos)
{
    tb_samples sample_sgt(_self, name);
    eosio_assert(sample_sgt.exists(), "game not found by this symbol name");

    st_sample sample = sample_sgt.get();
    eosio_assert(now() > sample.start_time, "the token issuance has not yet begun");

    int64_t stake = sample.buy(eos);

    eosio_assert(stake > 0, "stake amount should be bigger than 0");
    eosio_assert(stake < sample.base_stake - sample.stake, "stake amount overflow");

    sample_sgt.set(sample, sample.owner);

    return asset();
}

void dexlize::sample::version()
{

}

void dexlize::sample::transfer(account_name from, account_name to, asset quantity, string memo)
{
    require_auth(from);
    eosio_assert(is_account(to), "to account does not exist");
    eosio_assert(from != to, "cannot transfer to self");
    auto symbol_name = quantity.symbol.name();

    tb_samples sample_sgt(_self, symbol_name);
    eosio_assert(sample_sgt.exists(), "sample not found by this symbol name");
    st_sample sample = sample_sgt.get();

    require_recipient(from);
    require_recipient(to);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol == sample.symbol, "symbol precision mismatch");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    _sub_balance(from, quantity);
    _add_balance(to, quantity, from);
}

void dexlize::sample::buy(account_name from, account_name to, asset eos, string memo)
{
    if (from == _self || to != _self) return;
    eosio_assert(eos.symbol == CORE_SYMBOL, "must pay with CORE token");

    // find the account of owner
    string owner_str = "";
    auto iter = memo.find("-owner:");
    if (iter != string::npos) {
        owner_str = memo.substr(memo.find(":", iter) + 1);
        owner_str = owner_str.substr(0, owner_str.find("-"));
    }

    // find the account of referrer
    string referrer_str = "";
    auto refer_iter = memo.find("-referrer:");
    if (iter != string::npos) {
        referrer_str = memo.substr(memo.find(":", iter) + 1);
        referrer_str = referrer_str.substr(0, referrer_str.find("-"));
    }

    // transfer the referrer fee
    int32_t fee = eos.amount * 0.0001;
    account_name referrer = (referrer_str == "" || N(referrer_str) == from) ? REFERRER : N(referrer_str);
    action(permission_level{_self, N(active)},
        N(eosio.token),
        N(transfer),
        make_tuple(_self, N(referrer), asset(fee), string("sample refer fee https://dexlize.org"))
    ).send();

    // transfer the bought stake
    tb_samples sample_sgt(_self, eos.symbol.name);
    st_sample sample = sample_sgt.get();
    asset stake = asset(sample.buy(eos.amount - fee), sample.symbol);
    account_name owner = owner_str == "" ? from : N(owner_str);
    _add_balance(owner, stake, from);

    action(permission_level{_self, N(active)},
        _self,
        N(receipt),
        make_tuple(from, string("buy"), eos, stake, asset(fee))
    ).send();
}

void dexlize::sample::sell(account_name from, asset stake, string memo) 
{
    require_auth(from);
    accounts from_acnts(_self, from);
    auto from_acnt = from_acnts.find(stake.symbol.name());
    eosio_assert(from_acnt != from_acnts.end(), "account not found");
    eosio_assert(stake.symbol == from_acnt->balance.symbol, "symbol precision mismatch");
    eosio_assert((stake.amount > 0) && (stake.amount <= from_acnt->balance.amount), "invalid amount");

    asset eos_quantity, all_quantity;
    tie(eos_quantity, all_quantity);
    action(permission_level{_self, N(active)},
        _self,
        N(receipt),
        make_tuple(from, string("sell"), stake, all_quantity, all_quantity - eos_quantity)
    ).send();
}

void dexlize::sample::create(account_name issuer, asset maximum_supply)
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

void dexlize::sample::issue(account_name to, asset quantity, string memo)
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

void dexlize::sample::newsample(account_name from, asset base_eos_quantity, asset maximum_stake, 
        uint32_t lock_up_period, uint8_t base_fee_percent, uint8_t init_fee_percent, uint32_t start_time)
{
    require_auth(from);
    _init_sample(from, base_eos_quantity, maximum_stake, lock_up_period, base_fee_percent, init_fee_percent, start_time);
    
    SEND_INLINE_ACTION(*this, create, {from, N(active)}, {from, maximum_stake});
    SEND_INLINE_ACTION(*this, issue, {from, N(active)}, {from, maximum_stake, string("")});
}

extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        dexlize::sample thiscontract(receiver);

        if((code == N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &dexlize::sample::buy);
            return;
        }

        if (code != receiver) return;

        switch (action) {
            EOSIO_API(dexlize::sample, (version)(transfer)(sell)(newsample))
        };
        eosio_exit(0);
    }
}
