#include "uri/uri.h"
#include <iostream>

int main(int argc, char** argv) {
    tihi::Uri::ptr uri = tihi::Uri::Create("http://www.sylar.top/test/uri?id=102&name=ccc#frg");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}