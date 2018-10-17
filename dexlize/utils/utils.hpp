/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once

#include <string>
#include <vector>
#include <map>

namespace Dexlize {
    enum ErrorType {
        success = 0,
        fail1 = 1, // {} is not match
        fail2 = 2, // : is not match
        fail3 = 3, // "" is not macth
        unknow = 4,
    };

    class Utils {
        public:
        ErrorType parseJson(const std::string& memo, std::map<std::string, std::string>& memoMap);

        private:
        std::vector<std::string> _split(const std::string& str, const std::string& delim);
        std::string _trim(const std::string& str);
        std::string _removeSurplus(const std::string& str);
    };
};