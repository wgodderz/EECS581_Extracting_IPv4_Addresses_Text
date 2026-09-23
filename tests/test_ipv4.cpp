/**
 * @file test_ipv4.cpp
 * @brief Self checking test suite for the IPv4 extraction component.
 * @author Will Godderz
 * @date 2026-09-23
 *
 * runs extractIPv4 against a fixed table of inputs that covers the sample run
 * from the assignment, the boundary values on both ends of every field, the
 * leading zero rules, malformed structures, and the scanning behavior when one
 * line holds several candidates. each row carries its own expected answer plus
 * a short note on what it is guarding, so a failure says why it matters.
 *
 * build it with
 *   g++ -std=c++11 -Wall -Wextra -Isrc src/ipv4_extractor.cpp tests/test_ipv4.cpp -o tests/run_tests
 * then run ./tests/run_tests, where an exit code of 0 means everything passed.
 *
 * Inputs:  none, every case is baked into the table below.
 * Outputs: a pass or fail line per case, a final tally, and an exit code of 0
 *          when all cases passed or 1 when any of them did not.
 *
 * External sources:
 *   - syntax logic written with assistance from Claude (Anthropic), a
 *     generative AI assistant, on 2026-09-23.
 */

#include <iostream>
#include <string>

#include "ipv4_extractor.h"

namespace {

// one row of the table below. keeping the expectations right next to the input
// means adding a new case is a one line change instead of a new block of code.
struct TestCase {
    const char*   input;
    bool          expectFound;
    unsigned long expectAddress;   // only looked at when expectFound is true
    int           expectPort;      // -1 stands for no port
    const char*   why;             // printed on failure so the reason is obvious
};

const TestCase kCases[] = {
    // cases lifted straight out of the sample run in the assignment
    { "connecting to 192.168.1.1 now",      true,  3232235777UL, -1,   "plain address surrounded by words" },
    { "server=10.0.0.255:8080end",          true,   167772415UL, 8080, "address with port, no spaces around it" },
    { "192a168.1.1.1",                      true,  2818638081UL, -1,   "letter splits the text into two tokens and the second one is valid" },
    { "192.168.1.1.",                       false,          0UL, -1,   "trailing period is part of the token, so the token is malformed" },
    { "Connection from 192.168.1.1 refused",true,  3232235777UL, -1,   "address in the middle of a sentence" },
    { "192.168.01.1",                       false,          0UL, -1,   "leading zero in an octet" },
    { "1.2.3.4:99999",                      false,          0UL, -1,   "port out of range rejects the address too" },
    { "12.34.56",                           false,          0UL, -1,   "only three octets" },
    { "no number here",                     false,          0UL, -1,   "no token characters at all" },

    // both ends of every numeric field, since off by one errors like to hide here
    { "0.0.0.0",                            true,           0UL, -1,   "all zero address is valid and a single 0 is not a leading zero" },
    { "255.255.255.255",                    true, 4294967295UL, -1,    "largest address" },
    { "1.2.3.4:0",                          true,  16909060UL,   0,    "port 0 is in range and is not the same as having no port" },
    { "1.2.3.4:65535",                      true,  16909060UL, 65535,  "largest port" },
    { "1.2.3.4:65536",                      false,          0UL, -1,   "port one past the limit" },
    { "256.1.1.1",                          false,          0UL, -1,   "octet one past the limit" },
    { "255.255.255.256",                    false,          0UL, -1,   "last octet out of range" },

    // padded zeros get rejected everywhere, but a bare zero is still fine
    { "01.2.3.4",                           false,          0UL, -1,   "leading zero in the first octet" },
    { "1.2.3.04",                           false,          0UL, -1,   "leading zero in the last octet" },
    { "00.1.2.3",                           false,          0UL, -1,   "00 counts as a leading zero even though the value is 0" },
    { "1.2.3.4:080",                        false,          0UL, -1,   "leading zero in the port" },
    { "0.0.0.0:0",                          true,           0UL,  0,   "zeros everywhere and all of them legal" },

    // structural damage, mostly aimed at the rule that a whole token must match
    { "1.2.3.4.5",                          false,          0UL, -1,   "five octets, and nothing gets trimmed to find a good address inside" },
    { ".1.2.3.4",                           false,          0UL, -1,   "stray leading period is part of the token" },
    { ":1.2.3.4",                           false,          0UL, -1,   "stray leading colon is part of the token" },
    { "1.2.3.4:",                           false,          0UL, -1,   "colon with no port digits behind it" },
    { "1.2.3.4:80:90",                      false,          0UL, -1,   "second colon" },
    { "1.2:3.4.5.6",                        false,          0UL, -1,   "colon showing up before the fourth octet" },
    { "1..2.3.4",                           false,          0UL, -1,   "empty octet" },
    { "1.2.3.4::80",                        false,          0UL, -1,   "doubled colon" },
    { "....",                               false,          0UL, -1,   "punctuation only" },
    { "1234.5.6.7",                         false,          0UL, -1,   "four digit octet" },
    { "1.2.3.4567",                         false,          0UL, -1,   "four digit last octet, not quietly truncated to 456" },
    { "1.2.3.4:123456",                     false,          0UL, -1,   "six digit port, not quietly truncated to 12345" },
    { "",                                   false,          0UL, -1,   "empty line" },
    { "192.168.1",                          false,          0UL, -1,   "truncated address" },

    // scanning behavior, meaning garbage, several candidates, first valid wins
    { "1.2.3.4x5.6.7.8",                    true,   16909060UL, -1,    "two valid tokens and the first one wins" },
    { "bad 1.2.3.4:99999 good 5.6.7.8",     true,   84281096UL, -1,    "scan keeps going past a token that failed validation" },
    { "x1.2.3.4.5 y9.8.7.6",                true,  151521030UL, -1,    "malformed token skipped and a later valid token found" },
    { "1.2.3.4 :80",                        true,   16909060UL, -1,    "space breaks the token, so the port never attaches" },
    { "[2026-09-23] src=8.8.8.8:53 ok",     true,  134744072UL,   53,  "log style line with digits in the timestamp sitting first" },
    { "\t\t 172.16.254.1\t",                true, 2886794753UL, -1,    "tabs are garbage and get skipped" },
    { "999.999.999.999 then 1.1.1.1",       true,   16843009UL, -1,    "token with every octet out of range gets skipped" },
    { "END",                                false,          0UL, -1,   "the quit word holds no address, and main handles END rather than the parser" },
    { "ver1.2.3.4.5.6.7.8end",              false,          0UL, -1,   "one long malformed run with nothing valid inside it" },
    { "10.0.0.1:8080:9090 10.0.0.2:80",     true,  167772162UL,  80,   "first token has two colons and the second token is fine" }
};

// worked out from the table itself, so adding a row never needs a second edit
const int kCaseCount = static_cast<int>(sizeof(kCases) / sizeof(kCases[0]));

} // namespace

int main()
{
    int failures = 0;

    for (int i = 0; i < kCaseCount; ++i) {
        const TestCase& t = kCases[i];

        unsigned long address = 12345UL;   // poisoned on purpose, because the
        int           port    = 999;       // function has to overwrite both of
                                           // them even when it finds nothing

        const bool found = extractIPv4(std::string(t.input), address, port);

        // the found flag always has to line up
        bool ok = (found == t.expectFound);

        // on a hit, the address and the port both have to be exactly right
        if (ok && found) {
            ok = (address == t.expectAddress && port == t.expectPort);
        }

        // on a miss, the outputs have to be scrubbed back to the documented
        // failure values, which is the thing those poisoned values prove
        if (ok && !found) {
            ok = (address == 0UL && port == -1);
        }

        if (ok) {
            std::cout << "PASS  [" << t.input << "]" << std::endl;
        } else {
            // dump what was wanted next to what actually came back, so a
            // failure can be read without opening the parser
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

    // nonzero exit code so a build script or a grader can tell without reading
    return failures == 0 ? 0 : 1;
}
