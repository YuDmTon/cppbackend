#include "htmldecode.h"

#include <iostream>
#include <sstream>
#include <vector>

std::string HtmlDecode(std::string_view str) {
    using namespace std::literals;

    std::vector<std::string_view> mem = { 
        "&lt"sv,   "&LT"sv, 
        "&gt"sv,   "&GT"sv, 
        "&amp"sv,  "&AMP"sv,
        "&apos"sv, "&APOS"sv,
        "&quot"sv, "&QUOT"sv
    };
    std::vector<std::string_view> rep = { 
        "<"sv, "<"sv,  
        ">"sv, ">"sv,  
        "&"sv, "&"sv, 
        "'"sv, "'"sv, 
        "\""sv, "\""sv
    };
    //
    std::ostringstream oss;
    int pos    = 0;
    int found, found_2;
    while ( pos < str.size() ) {
        found = str.find("&"sv, pos);
        if ( found == std::string_view::npos ) {
            oss << str.substr(pos);
            break;
        }
        for (size_t j = 0; j < mem.size(); ++j) {
            found_2 = str.find(mem[j], found);
            if ( found_2 == found ) {
                oss << str.substr(pos, found - pos);
                oss << rep[j];
                pos = found + mem[j].size();
                if ( pos <= str.size() - 1 && str[pos] == ';' ) {
                    ++pos;
                }
                break;
            }
        }
        if ( found_2 != found ) {
            oss << str.substr(pos, found - pos + 1);
            pos = found + 1;
        }
    }
    return oss.str();
}
/*
    while ( pos < str.size() - 1 ) {
        for (size_t j = 0; j < mem.size(); ++j) {
            found = str.find(mem[j], pos);
std::cout << "j = " << j << ", pos = " << pos << ", find = '" << mem[j] << "', found = " << found << std::endl;
            if ( found != std::string_view::npos ) {
                oss << str.substr(pos, found - pos);
                oss << rep[j];
                pos = found + mem[j].size();
                if ( pos <= str.size() - 1 && str[pos] == ';' ) {
                    ++pos;
                }
std::cout << "found! new pos = " << pos << std::endl << std::endl;
                continue;
            }
        }
        if ( found == std::string_view::npos ) {
            oss << str.substr(pos);
            break;
        }
    }
*/