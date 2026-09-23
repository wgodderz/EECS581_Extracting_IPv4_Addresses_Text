#include <iostream>
#include <string>

#include "ipv4_extractor.h"

namespace {

// Input redirected from a Windows-style text file leaves a '\r' on the end of
// every line, which would make the "END" comparison fail. The '\r' is garbage
// to the parser either way, so it is simply dropped.
void stripTrailingCR(std::string& line)
{
    while (!line.empty() && (line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n')) {
        line.erase(line.size() - 1);
    }
}

void printResult(unsigned long address, int port)
{
    std::cout << "Extracted IPv4 address: "
              << ((address >> 24) & 0xFFUL) << '.'
              << ((address >> 16) & 0xFFUL) << '.'
              << ((address >>  8) & 0xFFUL) << '.'
              << ( address        & 0xFFUL)
              << " (decimal value: " << address << ", port: ";

    if (port < 0) {
        std::cout << "none";
    } else {
        std::cout << port;
    }

    std::cout << ")" << std::endl;
}

} // namespace

int main()
{
    std::string line;

    for (;;) {
        std::cout << "Enter a string (or 'END' to quit): ";

        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;     // end of input behaves like END
            break;
        }
        stripTrailingCR(line);

        if (line == "END") {            // case-sensitive, exact match
            break;
        }

        unsigned long address = 0;
        int port = -1;

        if (extractIPv4(line, address, port)) {
            printResult(address, port);
        } else {
            std::cout << "Invalid input: no valid IPv4 address found" << std::endl;
        }
    }

    std::cout << "Program terminated." << std::endl;
    return 0;
}
