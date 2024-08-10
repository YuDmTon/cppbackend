#include "urlencode.h"

#include <iomanip>
#include <sstream>

std::string UrlEncode(std::string_view str) {
    std::string_view spec_symb = "!#$%&'()*+,/:;=?@[]";
    std::ostringstream oss;
    for (size_t i = 0; i < str.size(); ++i) {
        char ch = static_cast<char>(str[i]);
        if ( ch <= 31 || ch >= 128 || spec_symb.find(ch) != std::string::npos ) {
            oss << "%" << std::hex << std::setw(2) << std::setfill('0') << (ch & 0xFF);
        } else if ( ch == ' ' ) {
            oss << "+";
        } else {
            oss << str[i];
        }
    }
    return oss.str();
}
