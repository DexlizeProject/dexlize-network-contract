#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

namespace Simple {
    class simple: public contract {
        public: 
        simple(account_name self): contract(self) {};

        void version();
        void newtoken();
        void transfer(account_name from, account_name to, asset quantity, string memo);
        void buy();
        void sell();

        private:
        void create(account_name issuer, asset maximum_supply);
        void issue(account_name to, asset quantity, string memo);

        private:
            struct account {
                asset    balance;

                uint64_t primary_key()const { return balance.symbol.name(); }
            };

            struct currency_stats {
                asset          supply;
                asset          max_supply;
                account_name   issuer;

                uint64_t primary_key()const { return supply.symbol.name(); }
            };

            typedef eosio::multi_index<N(accounts), account> accounts;
            typedef eosio::multi_index<N(stat), currency_stats> stats;
    };
}

#ifdef ABIGEN
    EOSIO_ABI(Simple::simple, (version)(transfer)(sell)(newtoken))
#endif