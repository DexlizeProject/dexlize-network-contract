/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once
#include "./utils.hpp"

#define SN(X) (string_to_symbol(0, #X) >> 8)
#define EOS_SYMBOL S(4, EOS)

#define GOD_ACCOUNT "eostestbp121"

#define ACTION_SELL_TYPE "sell"
#define ACTION_TRANSFER_TYPE "transfer"

namespace Dexlize {
    using namespace eosio;
    using namespace std;

    // @abi table accounts i64
    struct account {
        account_name name;
        vector<uint64_t> sells;
        vector<uint64_t> buys;

        uint64_t primary_key() const {return name;}
    };

    // @abi table sells i64
    struct tb_sell {
        uint64_t id;
        account_name name;
        extended_asset exchanged;
        extended_asset exchange;
        int64_t amount;
        bool actived;

        uint64_t primary_key() const {return id;}
    };

    // @abi table buys i64
    struct tb_buy {
        uint64_t id;
        account_name name;
        extended_asset exchanged;
        extended_asset exchange;
        int64_t amount;
        bool actived;

        uint64_t primary_key() const {return id;}
    };

    struct st_transfer {
        account_name from;
        account_name to;
        asset quantity;
        string memo;
    };

    // @abi table global i64
    struct st_global {
        bool running;
        uint64_t sell_id;
        uint64_t buy_id;

        uint64_t primary_key() const {return sell_id;}
    };

    typedef multi_index<N(accounts), account> tb_accounts;
    typedef multi_index<N(sells), tb_sell> tb_sells;
    typedef multi_index<N(buys), tb_buy> tb_buys;
    typedef singleton<N(global), st_global> global;
}