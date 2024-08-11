#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
    ////
    CHECK(HtmlDecode("1 &lt 2, 3 &LT 4, 5 &lt; 6, 7 &LT; 8, A &lT B"sv) == "1 < 2, 3 < 4, 5 < 6, 7 < 8, A &lT B"s);
    CHECK(HtmlDecode("2 &gt 1, 4 &GT 3, 6 &gt; 5, 8 &GT; 7, A &gT B"sv) == "2 > 1, 4 > 3, 6 > 5, 8 > 7, A &gT B"s);
    CHECK(HtmlDecode("&aposA&apos;, &APOSB&APOS;, &aPos1&ApoS;"sv) == "'A', 'B', &aPos1&ApoS;"s);
    CHECK(HtmlDecode("&quotA&quot;, &QUOTB&QUOT;, &qUot1&QuoT;"sv) == "\"A\", \"B\", &qUot1&QuoT;"s);
    CHECK(HtmlDecode("&abracadabra; &amp;lt"sv) == "&abracadabra; &lt"s);
}

// Напишите недостающие тесты самостоятельно
