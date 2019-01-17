/**
 *  @file
 *  @copyright defined in Dexlize/LICENSE
 */
#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/singleton.hpp>
#include <string>
#include <vector>
#include <map>

std::string _trim(const std::string& str) 
{
    std::string reVal = str;

    // remove left space
    for (auto i = 0; i < reVal.length(); ++i) {
        if (reVal[i] != ' ') {
            reVal = reVal.substr(i);
            break;
        }
    }

    // remove right space
    for (auto i = reVal.length() - 1; i > 0; --i) {
        if (reVal[i] != ' ') {
            reVal = reVal.substr(0, i + 1);
            break;
        }
    }

    return reVal;
}

std::vector<std::string> _split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> values;

    std::string t = str;
    std::string::size_type it = t.find(delim);
    while (it != std::string::npos) {
        values.emplace_back(_trim(t.substr(0, it)));
        t = t.substr(it + 1);
        it = t.find(delim);
    }
    if (t.length() > 0) {
        values.emplace_back(_trim(t));
    }

    return values;
}

std::string _removeSurplus(const std::string& str)
{
    std::string value = _trim(str);
    // remove the surplus character (e.g. "")
    if (value[0] == '"' && value[value.length() - 1] == '"') {
        value = value.substr(1, value.length() - 2);
    }
    return value;
}

bool parseJson(const std::string& memo, std::map<std::string, std::string>& memoMap)
{
    // remove surplus character(e.g. {}) in the format of memo, and then split the key_value by delimiter
    std::string::size_type t1 = memo.find("{"), t2 = memo.find("}");
    if (t1 != std::string::npos && t2 != std::string::npos) {
        std::vector<std::string> values =  _split(memo.substr(t1 + 1, t2 - 1), ",");

        // parse the key_value (e.g. "symbol": "DEX")
        for (auto it = values.begin(); it != values.end(); ++it) {
            if (it->find(":") == std::string::npos) return false;

            auto keyValue = _split(*it, ":");
            std::string key = _removeSurplus(keyValue[0]);
            std::string value = _removeSurplus(keyValue[1]);
            if (key.find('"') != std::string::npos || value.find('"') != std::string::npos) {
                return false;
            } else {
                memoMap.emplace(key, value);
            }
        }
    } 
    else {
        return false;
    }
    
    return true;
}