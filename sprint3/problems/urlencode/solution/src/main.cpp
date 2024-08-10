#include <iostream>

#include "urlencode.h"

int main() {
    using namespace std::literals;

    std::string s;
    std::getline(std::cin, s);

    std::cout << UrlEncode(s) << std::endl;
/*
    std::cout << "'" << UrlEncode(""sv) << "'" << std::endl;
    std::cout << "'" << UrlEncode("1234ABCDabcd<>"sv) << "'" << std::endl;
    std::cout << "'" << UrlEncode("!#$%&"sv) << "'" << std::endl;
    std::cout << "'" << UrlEncode("Hello, word !"sv) << "'" << std::endl;
    std::cout << "'" << UrlEncode("\x01\x02\x1F~\x80\x81"sv) << "'" << std::endl;
*/
}
