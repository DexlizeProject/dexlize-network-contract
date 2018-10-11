/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once

#include <map>
#include <string>

namespace Dexlize {
    class Utils {
        public:
        std::map<std::string, std::string> parseJson(const std::string& memo);
    };
};