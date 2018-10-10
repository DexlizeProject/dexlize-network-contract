/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>

#define TARGET_CONTRACT N(tokendapppub)

using namespace eosio;
using namespace std;

const account_name GOD_ACCOUNT = N(eostestbp121);

namespace Dexlize {
    class Proxy : public contract {
        public:
        Proxy(account_name self) : contract(self) {};
        void version();
        void buy(account_name from, account_name to, asset quantity, string memo);
        void sell();
        void convert();

        private:
        symbol_name _string_to_symbol_name(const char* str);
        map<string, string> _parseMemo(string memo);
    };
};

#ifdef ABIGEN
EOSIO_ABI(Dexlize::Proxy, (version)(buy)(sell))
#endif