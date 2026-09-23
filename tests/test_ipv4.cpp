// Self-checking test suite for extractIPv4.
// Build:  g++ -std=c++11 -Wall -Wextra -Isrc src/ipv4_extractor.cpp tests/test_ipv4.cpp -o tests/run_tests
// Run:    ./tests/run_tests        (exit code 0 = all passed)

#include <iostream>
#include <string>

#include "ipv4_extractor.h"

namespace {

struct TestCase {
    const char*   input;
    bool          expectFound;
    unsigned long expectAddress;   // ignored when expectFound is false
    int           expectPort;      // -1 == no port
    const char*   why;
};

const TestCase kCases[] = {
    // ---- cases taken straight from the assignment's sample run ----
    { "connecting to 192.168.1.1 now",      true,  3232235777UL, -1,   "plain address surrounded by words" },
    { "server=10.0.0.255:8080end",          true,   167772415UL, 8080, "address with port, no spaces around it" },
    { "192a168.1.1.1",                      true,  2818638081UL, -1,   "letter splits the text into two tokens; the second one is valid" },
    { "192.168.1.1.",                       false,          0UL, -1,   "trailing period is part of the token, so the token is malformed" },
    { "Connection from 192.168.1.1 refused",true,  3232235777UL, -1,   "address in the middle of a sentence" },
    { "192.168.01.1",                       false,          0UL, -1,   "leading zero in an octet" },
    { "1.2.3.4:99999",                      false,          0UL, -1,   "port out of range rejects the address too" },
    { "12.34.56",                           false,          0UL, -1,   "only three octets" },
    { "no number here",                     false,          0UL, -1,   "no token characters at all" },

    // ---- boundary values ----
    { "0.0.0.0",                            true,           0UL, -1,   "all-zero address is valid and a single 0 is not a leading zero" },
    { "255.255.255.255",                    true, 4294967295UL, -1,    "largest address" },
    { "1.2.3.4:0",                          true,  16909060UL,   0,    "port 0 is in range and is not 'no port'" },
    { "1.2.3.4:65535",                      true,  16909060UL, 65535,  "largest port" },
    { "1.2.3.4:65536",                      false,          0UL, -1,   "port one past the limit" },
    { "256.1.1.1",                          false,          0UL, -1,   "octet one past the limit" },
    { "255.255.255.256",                    false,          0UL, -1,   "last octet out of range" },

    // ---- leading zeros ----
    { "01.2.3.4",                           false,          0UL, -1,   "leading zero in the first octet" },
    { "1.2.3.04",                           false,          0UL, -1,   "leading zero in the last octet" },
    { "00.1.2.3",                           false,          0UL, -1,   "'00' is a leading zero even though the value is 0" },
    { "1.2.3.4:080",                        false,          0UL, -1,   "leading zero in the port" },
    { "0.0.0.0:0",                          true,           0UL,  0,   "zeros everywhere, all legal" },

    // ---- structural damage ----
    { "1.2.3.4.5",                          false,          0UL, -1,   "five octets: no trimming to find a valid address inside" },
    { ".1.2.3.4",                           false,          0UL, -1,   "stray leading period is part of the token" },
    { ":1.2.3.4",                           false,          0UL, -1,   "stray leading colon is part of the token" },
    { "1.2.3.4:",                           false,          0UL, -1,   "colon with no port digits" },
    { "1.2.3.4:80:90",                      false,          0UL, -1,   "second colon" },
    { "1.2:3.4.5.6",                        false,          0UL, -1,   "colon before the fourth octet" },
    { "1..2.3.4",                           false,          0UL, -1,   "empty octet" },
    { "1.2.3.4::80",                        false,          0UL, -1,   "doubled colon" },
    { "....",                               false,          0UL, -1,   "punctuation only" },
    { "1234.5.6.7",                         false,          0UL, -1,   "four-digit octet" },
    { "1.2.3.4567",                         false,          0UL, -1,   "four-digit last octet, not truncated to 456" },
    { "1.2.3.4:123456",                     false,          0UL, -1,   "six-digit port, not truncated to 12345" },
    { "",                                   false,          0UL, -1,   "empty line" },
    { "192.168.1",                          false,          0UL, -1,   "truncated address" },

    // ---- scanning behaviour: garbage, multiple candidates, first valid wins ----
    { "1.2.3.4x5.6.7.8",                    true,   16909060UL, -1,    "two valid tokens; the first one wins" },
    { "bad 1.2.3.4:99999 good 5.6.7.8",     true,   84281096UL, -1,    "scan continues past a token that failed validation" },
    { "x1.2.3.4.5 y9.8.7.6",                true,  151521030UL, -1,    "malformed token skipped, later valid token found" },
    { "1.2.3.4 :80",                        true,   16909060UL, -1,    "space breaks the token, so the port is not attached" },
    { "[2026-09-23] src=8.8.8.8:53 ok",     true,  134744072UL,   53,  "log-style line with digits in the timestamp first" },
    { "\t\t 172.16.254.1\t",                true, 2886794753UL, -1,    "tabs are garbage and are skipped" },
    { "999.999.999.999 then 1.1.1.1",       true,   16843009UL, -1,    "all-out-of-range token skipped" },
    { "END",                                false,          0UL, -1,   "the quit word contains no address (main handles END, not the parser)" },
    { "ver1.2.3.4.5.6.7.8end",              false,          0UL, -1,   "one long malformed run, nothing valid inside it" },
    { "10.0.0.1:8080:9090 10.0.0.2:80",     true,  167772162UL,  80,   "first token has two colons; second token is fine" }
};

const int kCaseCount = static_cast<int>(sizeof(kCases) / sizeof(kCases[0]));

} // namespace

int main()
{
    int failures = 0;

    for (int i = 0; i < kCaseCount; ++i) {
        const TestCase& t = kCases[i];

        unsigned long address = 12345UL;   // poisoned on purpose: the function
        int           port    = 999;       // must overwrite both on failure too

        const bool found = extractIPv4(std::string(t.input), address, port);

        bool ok = (found == t.expectFound);
        if (ok && found) {
            ok = (address == t.expectAddress && port == t.expectPort);
        }
        if (ok && !found) {
            ok = (address == 0UL && port == -1);   // required failure contract
        }

        if (ok) {
            std::cout << "PASS  [" << t.input << "]" << std::endl;
        } else {
            ++failures;
            std::cout << "FAIL  [" << t.input << "]  (" << t.why << ")" << std::endl
                      << "      expected: found=" << (t.expectFound ? "true" : "false")
                      << " address=" << t.expectAddress << " port=" << t.expectPort << std::endl
                      << "      actual:   found=" << (found ? "true" : "false")
                      << " address=" << address << " port=" << port << std::endl;
        }
    }

    std::cout << std::endl
              << (kCaseCount - failures) << " / " << kCaseCount << " tests passed."
              << std::endl;

    return failures == 0 ? 0 : 1;
}
