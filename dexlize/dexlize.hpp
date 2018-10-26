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

/*
    The Network contract: 
    
    Format of convert:
    [converter account, from token symbol, converter account, to token symbol...]
*/
namespace Dexlize {

    using namespace eosio;
    using namespace std;

    class Network : public contract {
        public:
        explicit Network(account_name self) : contract(self) {};
        void version();
        void buy(account_name from, account_name to, asset quantity, string memo);
        void sell(account_name from, account_name to, asset quantity, string memo);
        void convert(account_name from, asset quantity, string memo);

        private:
        Aux aux;
        map<uint64_t, vector<uint64_t>> mapContract2Symbol = {{N("tokendapppub"), {SN("PUB"), SN("TPT")}}};

        void _sendAction(account_name contract, account_name to, asset quantity, string actionStr, string memo);
        bool _checkSymbol(account_name contractAccount, symbol_name symbolName);
    };

    class Aux {
        public:
        account_name getContractAccount(const map<string, string>& memoMap) const;
        symbol_name getSymbolName(const map<string, string>& memoMap) const;
        string getOwnerAccount(const map<string, string>& memoMap) const;
        string getActionMemo(const symbol_name& symbolName, const string& owner) const;

        private:
        string _getMemoValue(const string& key, const map<string, string>& memoMap) const;
    };
}; // namespace dexlize

#ifdef ABIGEN
EOSIO_ABI(Dexlize::Network, (version))
#endif