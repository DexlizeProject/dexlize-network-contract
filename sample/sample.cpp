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

std::string dexlize::sample::_get_prop(string prop, string memo)
{
    string value = "";

    prop = "-" + prop + ":";
    auto iter = memo.find(prop);
    if (iter != string::npos) {
        value = memo.substr(memo.find(":", iter) + 1);
        value = value.substr(0, value.find("-"));
    }

    return value;
}

std::pair<eosio::asset, eosio::asset> dexlize::sample::_sample_sell(asset stake)
{
    tb_samples sample_sgt(_self, stake.symbol.name);
    eosio_assert(sample_sgt.exists(), "game not found by this symbol name");

    st_sample sample = sample_sgt.get();
    eosio_assert(now() > sample.start_time, "the token issuance has not yet begun");

    asset eos = asset(sample.sell(stake.amount), CORE_SYMBOL);
    asset fee = asset(sample.fee(eos.amount), CORE_SYMBOL);

    eosio_assert(eos.amount > 0, "eos amount should be bigger than zero");
    eosio_assert(eos.amount < sample.eos - sample.base_eos, "eos amount overflow");
    eosio_assert(fee.amount >= 0, "fee amount must be bigger than zero");

    sample_sgt.set(sample, sample.owner);

    return make_pair(eos, fee);
}

eosio::asset dexlize::sample::_sample_buy(symbol_name name, asset eos)
{
    tb_samples sample_sgt(_self, name);
    eosio_assert(sample_sgt.exists(), "game not found by this symbol name");

    st_sample sample = sample_sgt.get();
    eosio_assert(now() > sample.start_time, "the token issuance has not yet begun");

    asset stake = asset(sample.buy(eos.amount), sample.symbol);

    eosio_assert(stake.amount > 0, "stake amount should be bigger than zero");
    eosio_assert(stake.amount < sample.base_stake - sample.stake, "stake amount overflow");

    sample_sgt.set(sample, sample.owner);

    return stake;
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

    // find the bought symbol name from the memo
    string symbol_name_str = _get_prop("symbol", memo);

    // find the account of referrer from the memo
    string referrer_str = _get_prop("referrer", memo);

    // find the account of owner from the memo
    string owner_str = _get_prop("owner", memo);

    // transfer the referrer fee
    int32_t fee = eos.amount * 0.0001;
    account_name referrer = (referrer_str == "" || N(referrer_str) == from) ? REFERRER : N(referrer_str);
    action(permission_level{_self, N(active)},
        N(eosio.token),
        N(transfer),
        make_tuple(_self, N(referrer), asset(fee), string("sample refer fee https://dexlize.org"))
    ).send();

    // transfer the bought stake
    asset stake = _sample_buy(SN(symbol_name_str), eos);
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

    asset eos, fee;
    tie(eos, fee) = _sample_sell(stake);
    action(permission_level{_self, N(active)},
        _self,
        N(receipt),
        make_tuple(from, string("sell"), stake, eos, eos - fee)
    ).send();

    // find the account of owner from the memo
    string owner_str = _get_prop("owner", memo);
    account_name owner = owner_str == "" ? from : N(owner_str);
    action(permission_level{_self, N(active)},
        N(eosio.token),
        N(transfer),
        make_tuple(_self, owner, eos - fee, string("sample withdraw https://dexlize.org"))
    ).send();

    _sub_balance(from, stake);
}

void dexlize::sample::create(account_name issuer, asset maximum_supply)
{
    require_auth(_self);

    auto symbol = maximum_supply.symbol;
    eosio_assert(symbol.is_valid(), "invalid symbol name");
    eosio_assert(maximum_supply.is_valid(), "invalid supply");
    eosio_assert(maximum_supply.amount > 0, "max supply must be positive");

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
