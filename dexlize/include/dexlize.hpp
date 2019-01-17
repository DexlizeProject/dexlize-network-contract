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
            _global(_self, _self) {};

        void apply(const account_name& code, const action_name& action);
        void transfer(const account_name& from, const account_name& to, const extended_asset& quantity, const string& memo);

        // @abi action
        void create(const account_name& from, const string& memo);
        // @abi action
        void cancel(const account_name& from, const account_name& contract, uint64_t id, const string& memo);
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
        asset _toAsset(const string& amount);
        bool _parseMemo(const map<string, string>& memo, asset& exchanged, asset& exchange, account_name& contract);
        bool _parseMemo(const map<string, string>& memoMap, uint64_t& id, account_name& contract);
        void _sell(const account_name& from, const extended_asset& quantity, const account_name& contract, uint64_t id);
        void _buy(const account_name& from, const extended_asset& quantity, const account_name& contract, uint64_t id);
        void _activeSellOrder(const extended_asset& quantity, const account_name& contract, uint64_t id);
        void _activeBuyOrder(const extended_asset& quantity, const account_name& contract, uint64_t id);

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