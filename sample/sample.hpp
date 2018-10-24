#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/singleton.hpp>

typedef int int128_t;

#define REFERRER N(eostestbp121)

#define MAX_PERIOD 100ul * 365 * 24 * 60 * 60
#define MAX_QUANTITY 10000000000000ll * 10000
#define SN(X) (string_to_symbol(0, #X) >> 8)

/*
    The Sample contract:
    The Sample contract is the template of reference contract to interact dexlize network.
    It also allows developer to implement the content of own smart contact only if the current contract provide the common 
    interfaces {e.g. buy/sell/transfer} in the dexlize network.

    A note on transfer interface -
    Transfer interface in the smart contract of developer deployed can probid the other seller and buyer to transfer the 
    token, developer need to provide the authority of transfer for dexlize network.

    A note on the memo format -
    the memo format need to split by a dash delimited {e.g. "-symbol:DEX-owner:dexlize"}

    Format of convert:
    [converter account, from token symbol, converter account, to token symbol...]
*/
namespace Dexlize {

    using namespace eosio;
    using namespace std;

    class Sample: public contract {
        public: 
        explicit Sample(account_name self): contract(self) {};

        void version();
        void newsample(account_name from, asset base_eos_quantity, asset maximum_stake, 
        uint32_t lock_up_period, uint8_t base_fee_percent, uint8_t init_fee_percent, uint32_t start_time);
        void transfer(account_name from, account_name to, asset quantity, string memo);
        void buy(account_name from, account_name to, asset quantity, string memo);
        void sell(account_name from, asset quantity, string memo);

        private:
        void _sub_balance(account_name owner, asset value);
        void _add_balance(account_name owner, asset value, account_name ram_payer);
        string _get_prop(string prop, string memo);

        pair<asset, asset> _sample_sell(asset stake);
        asset _sample_buy(symbol_name name, asset eos);

        void _create(account_name issuer, asset maximum_supply);
        void _issue(account_name to, asset quantity, string memo);

        private:
        typedef eosio::multi_index<N(accounts), account> accounts;
        typedef eosio::multi_index<N(stat), currency_stats> stats;
        typedef singleton<N(sample), sample_market> market_samples;

        private:
        void _init_sample(account_name owner, asset base_eos_quantity, asset maximum_stake, uint32_t lock_up_period, 
                          uint8_t base_fee_percent, uint8_t init_fee_percent, uint32_t start_time);
    };

    struct account {
        asset    balance;

        uint64_t primary_key() const { return balance.symbol.name(); }
    };

    struct currency_stats {
        asset          supply;
        asset          max_supply;
        account_name   issuer;

        uint64_t primary_key() const { return supply.symbol.name(); }
    };

    struct sample_market {
        symbol_type symbol;
        account_name owner;
        int128_t eos;
        int64_t stake;
        int128_t base_eos;
        int64_t base_stake;
        uint32_t lock_up_period;
        uint8_t base_fee_percent;
        uint8_t init_fee_percent;
        uint32_t start_time;

        void _check() {
            eosio_assert(base_eos > 0, "failed to check base eos should be bigger than zero");
            eosio_assert(stake > 0, "failed to check stake should be bigger than zero");
            eosio_assert(eos >= base_eos, "failed to check eos is bigger than base eos");
            eosio_assert(base_stake >= stake, "failed to check base_stake is bigger than stake");
        }

        int64_t buy(int64_t buy_eos) {
            int64_t buy_stake = static_cast<int64_t>(buy_eos * stake / (buy_eos + eos));

            eos += buy_eos;
            stake -= buy_stake;
            _check();

            return buy_stake;
        }

        int64_t sell(int64_t sell_stake) {
            int64_t sell_eos = static_cast<int64_t>(sell_stake * eos / (stake + sell_stake));

            eos -= sell_eos;
            stake += sell_stake;

            return sell_eos;
        }

        int64_t fee(int64_t eos) {
            if ((init_fee_percent == base_fee_percent) || now() - start_time >= lock_up_period) return base_fee_percent;

            int64_t fee = 0;
            int64_t fee_ratio = static_cast<uint64_t>((1 - (now() - start_time) / lock_up_period) * init_fee_percent + 1);
            if (fee_ratio > 0 && stake < base_stake) {
                fee = (eos * fee_ratio + 99) / 100;
            }

            return fee;
        }
    };
}; // namespace Dexlize

#ifdef ABIGEN
    EOSIO_ABI(Dexlize::Sample, (version)(transfer)(sell)(newsimle))
#endif