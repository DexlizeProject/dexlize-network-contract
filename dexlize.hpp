/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>

using namespace eosio;
using namespace std;

#define SN(X) (string_to_symbol(0, #X) >> 8)
#define PUB_SYMBOL_NAME SN(PUB)
#define TPT_SYMBOL_NAME SN(TPT)
#define KBY_SYMBOL_NAME SN(KBY)

#define GOD_ACCOUNT "eostestbp121"
#define DAP_CONTRACT N(tokendapppub)
#define KBY_CONTRACT N(myeosgroupon)

namespace Dexlize {
    class Proxy : public contract {
        public:
        Proxy(account_name self) : contract(self) {};
        void version();
        void buy(account_name from, account_name to, asset quantity, string memo);
        void sell();
        void convert();

        private:
        symbol_name _string_to_symbol_name(const string& str);
        symbol_name _getSymbolName(const string& memo);
    };
};

#ifdef ABIGEN
EOSIO_ABI(Dexlize::Proxy, (version)(buy)(sell))
#endif