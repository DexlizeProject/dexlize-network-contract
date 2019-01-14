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

    class Aux {
        public:
        account_name getContractAccount(const map<string, string>& memoMap) const;
        symbol_name getSymbolName(const map<string, string>& memoMap) const;
        string getOwnerAccount(const map<string, string>& memoMap) const;
        string getActionMemo(const symbol_name& symbolName, const string& owner) const;

        private:
        string _getMemoValue(const string& key, const map<string, string>& memoMap) const;
    };

    class Network : public contract {
        public:
        explicit Network(account_name self) : 
            contract(self), 
            _accounts(_self, _self), 
            _sells(_self, _self),
            _buys(_self, _self),
            _global(_self, _self) {};

        void apply(const account_name& code, const action_name& action);
        void transfer(const account_name& from, const account_name& to, const extended_asset& quantity, const string& memo);

        // @abi action
        void create(const account_name& from, const account_name& token, const string& memo);
        // @abi action
        void cancel(const account_name& from, const uint64_t& bill_id, const string& memo);
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
        bool _parseMemo(const map<string, string>& memo, symbol_name& symbol, double& amount, account_name& contract);
        void _sell(uint64_t id, const account_name& from, const extended_asset& quantity);
        void _buy(uint64_t id, const account_name& from, const extended_asset& quantity);
        void _sellOrder(const account_name& from, const extended_asset& quantity, const account_name& contract, const symbol_name& symbol, int64_t amount);
        void _buyOrder(const account_name& from, const extended_asset& quantity, const account_name& contract, const symbol_name& symbol, int64_t amount);

        uint64_t _next_sell_id() {
            st_global global = _global.get_or_default(
                st_global{.sell_id = 0, .buy_id = 0});
            global.sell_id += 1;
            _global.set(global, _self);
            return global.sell_id;
        }

        uint64_t _next_buy_id() {
            st_global global = _global.get_or_default(
                st_global{.sell_id = 0, .buy_id = 0});
            global.buy_id += 1;
            _global.set(global, _self);
            return global.buy_id;
        }

        private:
        accounts _accounts;
        sells _sells;
        buys _buys;
        global _global;
    };
}; // namespace dexlize

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        Dexlize::Network thiscontract(receiver);
        thiscontract.apply(code, action);
        eosio_exit(0);
    }
}