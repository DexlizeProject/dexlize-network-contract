/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>

#define SN(X) (string_to_symbol(0, #X) >> 8)

#define GOD_ACCOUNT "eostestbp121"

#define ACTION_SELL_TYPE "sell"
#define ACTION_TRANSFER_TYPE "transfer"

namespace Dexlize {
    
    using namespace eosio;
    using namespace std;

    class Proxy : public contract {
        public:
        Proxy(account_name self) : contract(self) {};
        void version();
        void buy(account_name from, account_name to, asset quantity, string memo);
        void sell(account_name from, account_name to, asset quantity, string memo);

        private:
        Aux aux;
        map<uint64_t, vector<uint64_t>> mapContract2Symbol = {{N("tokendapppub"), {SN("PUB"), SN("TPT")}}};

        void _sendAction(account_name contract, account_name to, asset quantity, string actionStr, string memo);
        bool _checkSymbol(account_name contractAccount, symbol_name symbolName);
    };

    class Aux {
        public:
        account_name getContractAccount(const map<string, string>& memoMap);
        symbol_name getSymbolName(const map<string, string>& memoMap);
        string getOwnerAccount(const map<string, string>& memoMap);
        string getActionMemo(symbol_name symbolName, string owner);

        private:
        string _getMemoValue(const string& key, const map<string, string>& memoMap);
    };
};

#ifdef ABIGEN
EOSIO_ABI(Dexlize::Proxy, (version))
#endif