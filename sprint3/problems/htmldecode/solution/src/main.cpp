#include "htmldecode.h"
//
#include <iostream>

int main() {
    using namespace std::literals;
  
    std::string str;
    std::getline(std::cin, str);
    std::cout << HtmlDecode(str) << std::endl;
/*
    std::cout << HtmlDecode("1 &lt 2, 3 &LT 4, 5 &lt; 6, 7 &LT; 8, A &lT B, &"sv) << std::endl;
    std::cout << HtmlDecode("2 &gt 1, 4 &GT 3, 6 &gt; 5, 8 &GT; 7, A &gT B, &l"sv) << std::endl;
    std::cout << HtmlDecode("1 &amp 2, 3 &AMP 4, 5 &amp; 6, 7 &AMP; 8, A &aMp; B, &lt"sv) << std::endl;
    std::cout << HtmlDecode("&aposA&apos;, &APOSB&APOS; &aPos1&ApoS;, &lt;"sv) << std::endl;
    std::cout << HtmlDecode("&quotA&quot;, &QUOTB&QUOT; &qUot1&QuoT;"sv) << std::endl;
    std::cout << HtmlDecode("&abracadabra; &amp;lt"sv) << std::endl;
*/
}