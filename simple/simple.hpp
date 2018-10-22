#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/singleton.hpp>

typedef int int128_t;

namespace Simple {

    using namespace eosio;
    using namespace std;

    class simple: public contract {
        public: 
        simple(account_name self): contract(self) {};

        void version();
        void newtoken(account_name from, asset maximum_stake);
        void transfer(account_name from, account_name to, asset quantity, string memo);
        void buy();
        void sell();

        private:
        void create(account_name issuer, asset maximum_supply);
        void issue(account_name to, asset quantity, string memo);

        private:
            struct account {
                asset    balance;

                uint64_t primary_key() const { return balance.symbol.name(); }
            };

            struct currency_stats {
                asset          supply;
                asset          max_supply;
                account_name   issuer;

                uint64_t primary_key()const { return supply.symbol.name(); }
            };

            typedef eosio::multi_index<N(accounts), account> accounts;
            typedef eosio::multi_index<N(stat), currency_stats> stats;

            struct st_simple {
                symbol_type symbol;
                account_name owner;
                int128_t eos;
                int64_t stake;
                int64_t base_stake;
                uint32_t start_time;
                uint32_t lock_up_period;

                int64_t buy() {

                }

                int64_t sell() {

                }

                int64_t fee() {

                }
            };
            typedef singleton<N(simple), st_simple> tb_simples;
    };
}

#ifdef ABIGEN
    EOSIO_ABI(Simple::simple, (version)(transfer)(sell)(newtoken))
#endif