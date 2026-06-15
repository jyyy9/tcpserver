#include <regex>
#include <iostream>


int main()
{
    std::regex re(R"(^([A-Z]+)\s+(/[^?#]*)(\?([^#\s]*))?\s+HTTP/(\d+\.\d+)$)");
    std::smatch match;
    std::string line = "GET /path?query=value HTTP/1.1";
    if (std::regex_match(line, match, re)) {
        std::cout << match[1].str() << std::endl;
        std::cout << match[2].str() << std::endl;
        std::cout << match[3].str() << std::endl;
        std::cout << match[4].str() << std::endl;
        std::cout << match[5].str() << std::endl;
    }
    return 0;
}