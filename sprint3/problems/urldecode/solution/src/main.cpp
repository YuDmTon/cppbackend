#include <iostream>

#include "urldecode.h"

int main() {
    using namespace std::literals;

    try {
        std::string s;
        std::getline(std::cin, s);

        std::cout << UrlDecode(s) << std::endl;
/*
        std::cout << "'" << UrlDecode(""sv) << "'" << std::endl;
        std::cout << "'" << UrlDecode("1234567890abcdefABCDEF"sv) << "'" << std::endl;
        std::cout << "'" << UrlDecode(" !#$&'()*+,/:;=?@[]"sv) << "'" << std::endl;
        std::cout << "'" << UrlDecode("4 special symbols \"%21%23%24%26\""sv) << "'" << std::endl;
        std::cout << "'" << UrlDecode("4+special symbols+\"%21%23%24%26\""sv) << "'" << std::endl;
        //
//        std::cout << "'" << UrlDecode("Error 1+\"%C%c1%C2\""sv) << "'" << std::endl;
//        std::cout << "'" << UrlDecode("Error 2+\"%%c1%C2\""sv) << "'" << std::endl;
//        std::cout << "'" << UrlDecode("Error 3+\"%C0%c1%C\""sv) << "'" << std::endl;
//        std::cout << "'" << UrlDecode("Error 4+\"%C0%c1%\""sv) << "'" << std::endl;
//        std::cout << "'" << UrlDecode("Error 5 \"%Cx%c1%C2\""sv) << "'" << std::endl;
//        std::cout << "'" << UrlDecode("Error 6 \"%CX%c1%C2\""sv) << "'" << std::endl;
//        std::cout << "'" << UrlDecode("Error 7+\"%x0%c1%C2\""sv) << "'" << std::endl;
//        std::cout << "'" << UrlDecode("Error 8+\"%X0%c1%C2\""sv) << "'" << std::endl;
*/
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Error: "sv << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error"sv << std::endl;
    }

    return EXIT_FAILURE;
}
