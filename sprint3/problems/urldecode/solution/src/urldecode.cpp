#include "urldecode.h"

//#include <charconv>
#include <sstream>
#include <stdexcept>

std::string UrlDecode(std::string_view str) {
    std::string_view spec_symb = "!#$&'()*+,/:;=?@[]";
    std::ostringstream oss;
    for (size_t i = 0; i < str.size(); ++i)
	{
		if ( spec_symb.find(str[i]) != std::string::npos ) {
            //std::string err = "Unencoded character encountered \"" + str[i] + "\"";
            //throw std::invalid_argument(err);
        }
        if (str[i] == '%')
		{
            unsigned    u;
            std::string_view sub = str.substr(i + 1, 2);
            if ( i + 1 >= str.size() || i + 2 >= str.size() ) {
                std::string err = "Too short str";
                throw std::invalid_argument(err.c_str());
            }
            if ( !std::isxdigit(str[i + 1]) || !std::isxdigit(str[i + 2]) ) {
                std::string err = "\"" + std::string(sub) + "\" not hex string";
                throw std::invalid_argument(err.c_str());
            }
			std::stringstream ss;
            ss << std::hex << sub;
            ss >> u;
            oss << static_cast<char>(u);
			i += 2;
		}
		else if  (str[i] == '+') {
            oss << ' ';
        } else {
			oss << str[i];
		}
	}
	return oss.str();        
}
