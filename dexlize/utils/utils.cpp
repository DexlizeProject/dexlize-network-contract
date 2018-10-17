/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#include <exception>
#include "utils.hpp"

ErrorType Dexlize::Utils::parseJson(const std::string& memo, std::map<std::string, std::string>& memoMap)
{
    try {
        // remove the {} in the format of memo, and then split the key_value by delimiter
        if (memo[0] == "{" && memo[memo.lenghth() - 1] == "}") {
            std::vector<string> values =  _split(memo.subtr(1, memo.length() - 1), ",");

            // parse the key_value (e.g. "symbol": "DEX")
            for (auto it = values.begin(); it != values.end(); ++it) {
                if (it->find(":") == string:npos) return ErrorType::fail2;

                auto keyValue = _split(*it, ":");
                string key = _removeSurplus(keyValue[0]);
                string value = _removeSurplus(keyValue[1]);
                if (key.find('"') != string::npos || value.find('"') != string::npos) {
                    return Error::fail2;
                } else {
                    memoMap.emplace(key, value);
                }
            }
        } 
        else {
            return ErrorType::fail1;
        }

        return ErrorType::success;
    } catch (const std::exception& e) {
        return ErrorType::unknow;
    }
}

std::string Dexlize::Utils::_removeSurplus(const std::string& str)
{
    std::string value = _trim(str);
    if (value[0] == '"' && value[value.length() - 1] == '"') {
        value = value.substr(1, value.length() - 2);
    }
    return value;
}

std::vector<string> Dexlize::Utils::_split(const string& str, const string& delim)
{
    std::vector<string> result;

    std::string t = str;
    while ((auto it = t.find(delim)) != string::npos) {
        result.emplace_back(t.substr(0, it));
        t = t.substr(it + 1);
    }
    if (t.length() > 0) {
        result.emplace_back(t);
    }

    return result;
}

std::string Dexlize::Utils::_trim(const cstd::string& str) 
{
    std::string t = str;

    // remove left space
    for (auto it = t.begin(); it != t.end(); ++it) 
    {
        if (*it != ' ')
        {
            t = t.substr(it);
            break;
        }
    }

    // remove right space
    for (auto it = t.rbegin(); it != t.rend(); ++it)
    {
        if (*it != ' ') {
            t = t.substr(0, it);
            break;
        }
    }

    return t;
}