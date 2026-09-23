/**
 * @file main.cpp
 * @brief Interactive driver for the IPv4 extraction program.
 * @author Will Godderz
 * @date 2026-09-23
 *
 * handles the user facing half of the assignment. it loops reading lines from
 * standard input, hands each one to extractIPv4, and prints either the address
 * it found or an error line. the loop stops on the exact word END or on end of
 * input, so the program behaves the same whether someone types at it or a file
 * gets piped in.
 *
 * Inputs:  lines typed on standard input, one string per line.
 * Outputs: a formatted result line per input on standard output, or an invalid
 *          input message, followed by a termination message at the end.
 *
 * External sources:
 *   - syntax logic written with assistance from Claude (Anthropic), a
 *     generative AI assistant, on 2026-09-23.
 */

#include <iostream>
#include <string>

#include "ipv4_extractor.h"

// helpers only main needs, so they stay local to this file
namespace {

// input redirected out of a windows style text file leaves a carriage return
// stuck on the end of every line, which would quietly break the END check.
// the parser treats that character as garbage anyway, so the easiest fix is to
// shave it off before anything else looks at the line.
void stripTrailingCR(std::string& line)
{
    while (!line.empty() && (line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n')) {
        line.erase(line.size() - 1);
    }
}

// prints one successful match in the format the assignment asks for. the
// address comes back packed into a single value, so it gets pulled apart one
// octet at a time with a shift and a mask before being printed.
void printResult(unsigned long address, int port)
{
    std::cout << "Extracted IPv4 address: "
              << ((address >> 24) & 0xFFUL) << '.'
              << ((address >> 16) & 0xFFUL) << '.'
              << ((address >>  8) & 0xFFUL) << '.'
              << ( address        & 0xFFUL)
              << " (decimal value: " << address << ", port: ";

    // -1 is the sentinel meaning the text simply had no port on it, which is
    // different from a real port of 0 and has to read differently too
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

        // a failed getline means the stream dried up, usually a piped file
        // running out, so treat it exactly like the user typing END
        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;     // tidy up the dangling prompt
            break;
        }
        stripTrailingCR(line);

        if (line == "END") {            // case sensitive and has to be the whole line
            break;
        }

        // poisoned on purpose is not needed here, but starting clean keeps the
        // printing code honest if extractIPv4 ever returns without writing
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
