/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <vector>

#define SN(X) (string_to_symbol(0, #X) >> 8)

#define GOD_ACCOUNT "eostestbp121"

#define ACTION_SELL_TYPE "sell"
#define ACTION_TRANSFER_TYPE "transfer"

namespace Dexlize {
    using namespace eosio;
    using namespace std;

    // @abi table accounts i64
    struct account {
        account_name name;
        vector<extended_asset> tokens;

        uint64_t primary_key() const {return name;}
    }
    
    struct st_transfer {
        account_name from;
        account_name to;
        asset quantity;
        string memo;
    };

    typedef multi_index<N(accounts), account> accounts; 
}