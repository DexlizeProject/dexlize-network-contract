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

#define ACTION_CONVERT_TYPE "convert"
#define ACTION_SELL_TYPE "sell"
#define ACTION_TRANSFER_TYPE "transfer"

namespace Dexlize {
    class Proxy : public contract {
        public:
        Proxy(account_name self) : contract(self) {};
        void version();
        void buy(account_name from, account_name to, asset quantity, string memo);
        void sell(account_name from, account_name to, asset quantity, string memo);

        private:
        Aux aux;

        void _sendAction(account_name contract, account_name to, asset quantity, string actionStr, string memo = "");
    };

    class Aux {
        public:
        symbol_name string2SymbolName(const string& str);
        account_name getContractAccountName(symbol_name symbolName);
        string getActionMemo(symbol_name symbolName);
        symbol_name getSymbolName(const map<string, string>& memoMap);
        asset getStakeAmount(const map<string, string>& memoMap);
        asset getEosAmount(const map<string, string>& memoMap);
        string getTransactionType(const map<string, string>& memoMap);

        private:
        string _getMemoValue(const string& key, const map<string, string>& memoMap);
    };
};

#ifdef ABIGEN
EOSIO_ABI(Dexlize::Proxy, (version))
#endif