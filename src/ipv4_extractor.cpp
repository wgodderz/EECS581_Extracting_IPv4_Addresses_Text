#include "ipv4_extractor.h"

// No <cstdlib>, <cstdio>, <regex>, or socket headers are included on purpose:
// every digit is accumulated by hand and every structural rule is checked by
// explicit character-by-character code.

namespace {

bool isDigitChar(char c)
{
    return c >= '0' && c <= '9';
}

// Only digits, '.' and ':' can ever be part of a candidate token.
// Everything else is garbage that separates one candidate from the next.
bool isTokenChar(char c)
{
    return isDigitChar(c) || c == '.' || c == ':';
}

// Reads one run of digits from s[i] while staying inside [i, end).
//
// maxDigits - how many digits the field is allowed to have (3 for an octet,
//             5 for a port); one more than that is a failure, not a truncation.
// limit     - largest value the field may hold (255 or 65535).
//
// On success the value is stored in `value`, `i` is advanced past the digits,
// and true is returned. On failure `i` is left wherever it stopped; the caller
// abandons the whole token anyway, so it never re-reads from that position.
bool readNumber(const std::string& s, std::string::size_type& i,
                std::string::size_type end, std::string::size_type maxDigits,
                long limit, long& value)
{
    const std::string::size_type start = i;
    long accumulator = 0;

    while (i < end && isDigitChar(s[i])) {
        if (i - start >= maxDigits) {
            return false;               // too many digits (e.g. "1234" or ":123456")
        }
        accumulator = accumulator * 10 + (s[i] - '0');   // manual digit accumulation
        ++i;
    }

    const std::string::size_type digits = i - start;

    if (digits == 0) {
        return false;                   // empty field, e.g. "1..2.3" or "1.2.3.4:"
    }
    if (digits > 1 && s[start] == '0') {
        return false;                   // disallowed leading zero, e.g. "01" or "00"
    }
    if (accumulator > limit) {
        return false;                   // out of range, e.g. 256 or 65536
    }

    value = accumulator;
    return true;
}

// Validates ONE maximal token (the half-open range [start, end) of `s`) against
// the full grammar: octet.octet.octet.octet[:port].
//
// The match must consume the entire token. A token is never trimmed to find a
// valid address hiding inside it, which is what rejects "192.168.1.1." and
// "1.2.3.4.5" even though a valid address is visible inside them.
bool matchToken(const std::string& s, std::string::size_type start,
                std::string::size_type end, unsigned long& outAddress, int& outPort)
{
    std::string::size_type i = start;
    long octet[4];

    for (int k = 0; k < 4; ++k) {
        if (!readNumber(s, i, end, 3, 255, octet[k])) {
            return false;
        }
        if (k < 3) {
            if (i >= end || s[i] != '.') {
                return false;           // missing separator, or ':' used too early
            }
            ++i;                        // consume the '.'
        }
    }

    int port = -1;

    if (i < end) {                      // something follows the fourth octet
        if (s[i] != ':') {
            return false;               // a trailing '.' - the token is malformed
        }
        ++i;                            // consume the ':'

        long portValue = 0;
        if (!readNumber(s, i, end, 5, 65535, portValue)) {
            return false;               // bad port rejects the whole match
        }
        if (i != end) {
            return false;               // trailing junk, e.g. "1.2.3.4:80:90"
        }
        port = static_cast<int>(portValue);
    }

    outAddress = (static_cast<unsigned long>(octet[0]) << 24)
               | (static_cast<unsigned long>(octet[1]) << 16)
               | (static_cast<unsigned long>(octet[2]) <<  8)
               |  static_cast<unsigned long>(octet[3]);
    outPort = port;
    return true;
}

} // namespace

bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort)
{
    outAddress = 0;
    outPort = -1;

    const std::string::size_type n = str.size();
    std::string::size_type i = 0;

    while (i < n) {
        if (!isTokenChar(str[i])) {
            ++i;                        // skip garbage
            continue;
        }

        // Take the MAXIMAL run of token characters, then require the whole run
        // to match. Grabbing only as much as fits the grammar would wrongly
        // accept "192.168.1.1." and "1.2.3.4.5".
        const std::string::size_type tokenStart = i;
        while (i < n && isTokenChar(str[i])) {
            ++i;
        }

        if (matchToken(str, tokenStart, i, outAddress, outPort)) {
            return true;                // first valid address wins
        }
        // matchToken only writes to the outputs on success, so they are still
        // 0 / -1 here and the scan continues after this failed token.
    }

    return false;
}
