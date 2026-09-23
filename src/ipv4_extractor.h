#ifndef IPV4_EXTRACTOR_H
#define IPV4_EXTRACTOR_H

#include <string>

// Scans `str` for the first valid IPv4 address (optionally followed by :port)
// embedded anywhere in the text.
//
// Returns true if a valid address was found, false otherwise.
// On success: outAddress holds the 32-bit value,
// and outPort holds the port number, or -1 if no port was present.
// On failure: outAddress is set to 0 and outPort is set to -1.
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort);

#endif // IPV4_EXTRACTOR_H
