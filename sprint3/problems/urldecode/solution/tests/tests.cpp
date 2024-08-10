#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    // Напишите остальные тесты для функции UrlDecode самостоятельно
    BOOST_TEST(UrlDecode("1234567890abcdefABCDEF"sv) == "1234567890abcdefABCDEF"s);
    BOOST_TEST(UrlDecode(" !#$&'()*,/:;=?@[]"sv) == " !#$&'()*,/:;=?@[]"s);
    BOOST_TEST(UrlDecode("4 special symbols %21%23%24%26"sv) == "4 special symbols !#$&"s);
    BOOST_TEST(UrlDecode("4+special symbols+%21%23%24%26"sv) == "4 special symbols !#$&"s);
//    BOOST_TEST(UrlDecode("Wrong encoded 1 %2%23%24%26"sv) == "Wrong encoded 1 !#$&"s);
//    BOOST_TEST(UrlDecode("Wrong+encoded 2+%H2%23%24%26"sv) == "Wrong encoded 2!#$&"s);
}