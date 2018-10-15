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
#define PUB_SYMBOL S(4, PUB)
#define PUB_SYMBOL_NAME SN(PUB)
#define TPT_SYMBOL_NAME SN(TPT)
#define KBY_SYMBOL_NAME SN(KBY)

#define GOD_ACCOUNT "eostestbp121"
#define EOSIO_CONTRACT N(eosio.token)
#define DAP_CONTRACT N(tokendapppub)
#define KBY_CONTRACT N(myeosgroupon)

namespace Dexlize {
    class Proxy : public contract {
        public:
        Proxy(account_name self) : contract(self) {};
        void version();
        void buy(account_name from, account_name to, asset quantity, string memo);
        void sell(account_name from, account_name to, asset quantity, string memo);
        void convert(account_name from, asset fromAsset, asset toAsset);

        private:
        Aux aux;
        // symbol_name _string2SymbolName(const string& str);
        // string _getMemoValue(const string& key, const map<string, string>& memoMap);
        // symbol_name _getSymbolName(const map<string, string>& memoMap);
        // asset _getStakeAmount(const map<string, string>& memoMap);
        // asset _getEosAmount(const map<string, string>& memoMap);
        // account_name _getContractAccountName(symbol_name symbolName);
        // string _getActionMemo(symbol_name symbolName);
    };

    class Aux {
        public:
        symbol_name string2SymbolName(const string& str);
        symbol_name getSymbolName(const map<string, string>& memoMap);
        asset getStakeAmount(const map<string, string>& memoMap);
        asset getEosAmount(const map<string, string>& memoMap);
        account_name getContractAccountName(symbol_name symbolName);
        string getActionMemo(symbol_name symbolName);

        private:
        string _getMemoValue(const string& key, const map<string, string>& memoMap);
    };
};

#ifdef ABIGEN
EOSIO_ABI(Dexlize::Proxy, (version)(sell))
#endif