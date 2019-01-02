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