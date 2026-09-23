/**
 * @file ipv4_extractor.cpp
 * @brief Implementation of the IPv4 extraction component.
 * @author Will Godderz
 * @date 2026-09-23
 *
 * implements the scanner declared in ipv4_extractor.h. the line is chopped
 * into maximal runs of digits, periods and colons, and every run is then
 * checked against the whole grammar octet.octet.octet.octet with an optional
 * port on the end. no library parsing helpers are used anywhere. every digit
 * is accumulated by hand and every structural rule is enforced with explicit
 * character by character code.
 *
 * Inputs:  one line of text handed over by the caller.
 * Outputs: true or false for whether a match was found, plus the packed
 *          address and port written through the out parameters.
 *
 * External sources:
 *   - syntax logic written with assistance from Claude (Anthropic), a
 *     generative AI assistant, on 2026-09-23.
 */

#include "ipv4_extractor.h"

// everything in here is private to this translation unit. the anonymous
// namespace keeps these helpers off the linker and out of anyone else's way.
namespace {

// plain ascii digit test. doing it by hand instead of reaching for isdigit
// keeps the behavior the same no matter what locale the program runs under.
bool isDigitChar(char c)
{
    return c >= '0' && c <= '9';
}

// digits, periods and colons are the only characters that can ever show up
// inside a candidate. anything else is garbage and acts as the fence between
// one candidate and the next, which is how "192a168.1.1.1" ends up being two
// separate runs rather than one broken one.
bool isTokenChar(char c)
{
    return isDigitChar(c) || c == '.' || c == ':';
}

// reads one run of digits out of s starting at i, never stepping past end.
//
// maxDigits is how many digits the field is allowed to hold, 3 for an octet
// and 5 for a port. going over that is a hard failure rather than a quiet
// truncation, which is the whole reason "1.2.3.4567" gets thrown out instead
// of being read as 456.
// limit is the biggest value the field may hold, so 255 or 65535.
//
// on success the number lands in value, i is parked just past the last digit
// and true comes back. on failure i is left wherever it gave up, which is
// harmless because the caller walks away from the entire run anyway and never
// tries to re read from that spot.
bool readNumber(const std::string& s, std::string::size_type& i,
                std::string::size_type end, std::string::size_type maxDigits,
                long limit, long& value)
{
    const std::string::size_type start = i;
    long accumulator = 0;

    // eat digits one at a time, building the value as we go
    while (i < end && isDigitChar(s[i])) {
        if (i - start >= maxDigits) {
            return false;               // too many digits, like "1234" or ":123456"
        }
        accumulator = accumulator * 10 + (s[i] - '0');   // manual digit accumulation
        ++i;
    }

    const std::string::size_type digits = i - start;

    // three ways a field can still be bad even after the loop is happy
    if (digits == 0) {
        return false;                   // nothing was there at all, like "1..2.3" or "1.2.3.4:"
    }
    if (digits > 1 && s[start] == '0') {
        return false;                   // padded with a leading zero, like "01" or "00"
    }
    if (accumulator > limit) {
        return false;                   // past the ceiling, so 256 or 65536
    }

    value = accumulator;
    return true;
}

// checks ONE maximal run, the half open range [start, end) of s, against the
// entire grammar octet.octet.octet.octet with an optional port after a colon.
//
// the match has to swallow the whole run. a run is never trimmed down looking
// for a good address hiding inside it, and that is exactly what kills
// "192.168.1.1." and "1.2.3.4.5" even though you can see a valid address in
// both of them.
bool matchToken(const std::string& s, std::string::size_type start,
                std::string::size_type end, unsigned long& outAddress, int& outPort)
{
    std::string::size_type i = start;
    long octet[4];

    // four octets, with a period required between each pair of them
    for (int k = 0; k < 4; ++k) {
        if (!readNumber(s, i, end, 3, 255, octet[k])) {
            return false;
        }
        if (k < 3) {
            if (i >= end || s[i] != '.') {
                return false;           // separator missing, or a colon showed up way too early
            }
            ++i;                        // step over the period
        }
    }

    int port = -1;                      // stays -1 when the text never had one

    if (i < end) {                      // there is still something after the fourth octet
        if (s[i] != ':') {
            return false;               // a trailing period, so the run is malformed
        }
        ++i;                            // step over the colon

        long portValue = 0;
        if (!readNumber(s, i, end, 5, 65535, portValue)) {
            return false;               // a bad port takes the whole match down with it
        }
        if (i != end) {
            return false;               // leftover junk on the end, like "1.2.3.4:80:90"
        }
        port = static_cast<int>(portValue);
    }

    // pack the four octets into one 32 bit value, most significant octet first
    outAddress = (static_cast<unsigned long>(octet[0]) << 24)
               | (static_cast<unsigned long>(octet[1]) << 16)
               | (static_cast<unsigned long>(octet[2]) <<  8)
               |  static_cast<unsigned long>(octet[3]);
    outPort = port;
    return true;
}

} // namespace

// see ipv4_extractor.h for what the caller is promised. the job here is just
// to find the runs worth checking and hand each one to matchToken.
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort)
{
    // clear these first so the failure contract holds no matter where we bail
    outAddress = 0;
    outPort = -1;

    const std::string::size_type n = str.size();
    std::string::size_type i = 0;

    while (i < n) {
        if (!isTokenChar(str[i])) {
            ++i;                        // ordinary garbage, letters, spaces, tabs, keep moving
            continue;
        }

        // grab the MAXIMAL run of token characters, then make the whole thing
        // match. taking only as much as the grammar likes would happily accept
        // "192.168.1.1." and "1.2.3.4.5", and both of those are supposed to lose.
        const std::string::size_type tokenStart = i;
        while (i < n && isTokenChar(str[i])) {
            ++i;
        }

        if (matchToken(str, tokenStart, i, outAddress, outPort)) {
            return true;                // first valid address wins, we are done
        }
        // matchToken only touches the outputs when it succeeds, so they are
        // still sitting at 0 and -1 here. i already points past the failed run,
        // which is how "bad 1.2.3.4:99999 good 5.6.7.8" still finds the second one.
    }

    return false;                       // walked the whole line and came up empty
}
