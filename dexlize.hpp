/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE.txt
 */
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

namespace Dexlize {
    class Proxy : public contract {
        public:
        Proxy(account_name self) : contract(self) {};

        private:

    };
};