#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    ////
    EXPECT_EQ(UrlEncode("1234ABCDabcd<>"sv), "1234ABCDabcd<>"s);
    EXPECT_EQ(UrlEncode("!#$%&"sv), "%21%23%24%25%26"s);
    EXPECT_EQ(UrlEncode("Hello, word !"sv), "Hello%2c+word+%21"s);
    EXPECT_EQ(UrlEncode("\x01\x02\x1F~\x80\x81"sv), "%x01%x02%x1f~%x80%x81"s);
}
