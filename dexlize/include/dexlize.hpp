/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once
#include "types.hpp"

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

        void apply(const account_name& code, const action_name& action);
        void transfer(account_name from, account_name to, extended_asset quantity, string memo);

        // @abi action
        void cancel(account_name from, uint64_t bill_id);
        // @abi action
        void version();
        // @abi action
        void buy(account_name from, account_name to, asset quantity, string memo);
        // @abi action
        void sell(account_name from, account_name to, asset quantity, string memo);
        // @abi action
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

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        xprime::manage thiscontract(receiver);
        thiscontract.apply(code, action);
        eosio_exit(0);
    }
}