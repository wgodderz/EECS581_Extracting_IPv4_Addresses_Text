/**
 * @file ipv4_extractor.h
 * @brief Public interface for the IPv4 extraction component.
 * @author Will Godderz
 * @date 2026-09-23
 *
 * declares the single function the rest of the program needs, which digs the
 * first valid ipv4 address out of an arbitrary line of text. all of the actual
 * parsing lives in ipv4_extractor.cpp, so anything that includes this header
 * only has to know about one entry point and two out parameters.
 *
 * Inputs:  one line of text of any length, empty lines included.
 * Outputs: a bool saying whether an address turned up, plus the packed 32 bit
 *          address and the port handed back through the reference parameters.
 *
 * External sources:
 *   - syntax logic written with assistance from Claude (Anthropic), a
 *     generative AI assistant, on 2026-09-23.
 */

#ifndef IPV4_EXTRACTOR_H
#define IPV4_EXTRACTOR_H

#include <string>

// walks str from left to right and hands back the first valid ipv4 address it
// runs into, with an optional port attached after a colon. the address can sit
// anywhere in the line, so something like "connecting to 10.0.0.1 now" is fine.
//
// returns true when a valid address was found and false when it was not.
// when it succeeds, outAddress holds the packed 32 bit value and outPort holds
// the port number, or -1 if the text never had a port on it.
// when it fails, both parameters are wiped back to 0 and -1, so a caller can
// never end up reading whatever junk it passed in.
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort);

#endif // IPV4_EXTRACTOR_H
