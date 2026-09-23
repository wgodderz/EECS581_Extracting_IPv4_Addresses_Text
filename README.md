# EECS 581 — Extracting IPv4 Addresses from Text

Extracts a single valid IPv4 address (with an optional `:port`) embedded anywhere in a
line of unstructured text, using hand-written character-by-character parsing.

- [Layout](#layout)
- [Build and run](#build-and-run)
- [Restrictions observed](#restrictions-observed)
- [How the parser works](#how-the-parser-works)
- [Output format](#output-format)
- [Test cases](#test-cases)
- [Generative AI disclosure](#generative-ai-disclosure)

## Layout

| Path | Contents |
| --- | --- |
| `src/ipv4_extractor.h` | Prototype for `extractIPv4`, exactly as specified in the assignment |
| `src/ipv4_extractor.cpp` | The parser: scanning, validation, manual digit accumulation |
| `src/main.cpp` | Input loop and output formatting |
| `tests/test_ipv4.cpp` | 45 self-checking test cases |
| `README.md` | This file: documentation, test-case rationale, and the required GAI disclosure |
| `Makefile`, `.gitignore` | Build rules; compiled binaries are not committed |

## Build and run

```
make            # builds ./ipv4 and ./tests/run_tests
make test       # runs the test suite (exit code 0 = all passed)
./ipv4          # interactive program
```

On Windows with MinGW the command is `mingw32-make` rather than `make`, and the binaries are
named `ipv4.exe` and `tests/run_tests.exe`.

Without `make`:

```
g++ -std=c++11 -Wall -Wextra -Isrc src/main.cpp src/ipv4_extractor.cpp -o ipv4
g++ -std=c++11 -Wall -Wextra -Isrc tests/test_ipv4.cpp src/ipv4_extractor.cpp -o tests/run_tests
```

Built and tested with MinGW g++ 6.3.0 on Windows 11. No warnings under `-Wall -Wextra`.

## Restrictions observed

No `atoi`/`strtol`/`sscanf`/`stoi`, no `inet_pton`/`inet_addr`, no `std::regex`. Digits are
accumulated with `accumulator = accumulator * 10 + (c - '0')`. The only library headers used
are `<string>` and `<iostream>`.

## How the parser works

1. **Scan.** Walk the line. Any character that is not a digit, `.` or `:` is garbage and is
   skipped.
2. **Tokenize.** On hitting a token character, take the *maximal* run of digits, `.` and `:`.
3. **Validate the whole run.** The run must match `octet.octet.octet.octet[:port]` from its
   first character to its last. No trimming, no partial match.
   - octet: 1–3 digits, 0–255, no leading zero unless the octet is exactly `0`
   - port: 1–5 digits, 0–65535, same leading-zero rule; if a `:` is present the port must be
     valid or the entire match (address included) is rejected
4. **First valid run wins.** A run that fails validation is abandoned and the scan resumes
   after it; a later run in the same line may still succeed.

The maximal-run rule in step 2 is what makes the assignment's own examples come out right:
`192.168.1.1.` is one run ending in a stray period, so it is rejected rather than trimmed to
`192.168.1.1`, while in `192a168.1.1.1` the letter `a` splits the text into `192` (invalid)
and `168.1.1.1` (valid).

## Output format

```
Extracted IPv4 address: A.B.C.D (decimal value: N, port: P)
Invalid input: no valid IPv4 address found
Program terminated.
```

`P` is the port number, or the literal text `none`. The loop runs until the user enters `END`
(case-sensitive).

---

# Test cases

45 cases, all in `tests/test_ipv4.cpp`. Run `make test`; the program exits non-zero if any case
fails. Each case checks three things: the return value, and *both* output parameters —
including on failure, where the contract requires `outAddress == 0` and `outPort == -1`. The
harness pre-poisons both outputs with junk values (`12345` and `999`) before every call, so a
function that simply forgets to write them cannot pass.

## 1. The assignment's own sample run (9 cases)

Every line of the sample run in the handout, with the exact expected address, decimal value
and port. These are the baseline — if any of these fail, nothing else matters.

## 2. Numeric limits (7 cases)

| Input | Expected | Point |
| --- | --- | --- |
| `0.0.0.0` | valid, 0 | lowest address; a bare `0` is not a leading zero |
| `255.255.255.255` | valid, 4294967295 | highest address; also checks the shift math does not overflow |
| `256.1.1.1` | rejected | one past the octet limit |
| `255.255.255.256` | rejected | the limit is checked on the *last* octet too, not just the first |
| `1.2.3.4:0` | valid, port 0 | port 0 is in range, and must be reported as `0`, not confused with "no port" |
| `1.2.3.4:65535` | valid | highest port |
| `1.2.3.4:65536` | rejected | one past the port limit |

## 3. Leading zeros (5 cases)

`01.2.3.4`, `1.2.3.04`, `00.1.2.3`, `1.2.3.4:080` are all rejected; `0.0.0.0:0` is accepted.
`00` is the interesting one: its *value* is 0, so a check written as "leading zero is only
allowed if the value is 0" accepts it wrongly. The rule has to be about the digits — more than
one digit and the first is `0` — not about the value.

## 4. Structural damage (14 cases)

Wrong octet count (`1.2.3.4.5`, `192.168.1`, `12.34.56`), empty octets (`1..2.3.4`), stray
punctuation touching a valid address (`.1.2.3.4`, `:1.2.3.4`, `192.168.1.1.`), colon problems
(`1.2.3.4:` with no digits, `1.2.3.4:80:90`, `1.2.3.4::80`, `1.2:3.4.5.6` where the colon comes
too early), oversized digit runs (`1234.5.6.7`, `1.2.3.4567`, `1.2.3.4:123456`), and the
degenerate inputs `....` and `""`.

`1.2.3.4567` and `1.2.3.4:123456` specifically catch a parser that reads "up to 3 digits" and
then stops: it would happily return `1.2.3.456` / port `12345` and leave the rest behind. The
correct behaviour is to reject the token, because one more digit follows.

## 5. Scanning behaviour (10 cases)

| Input | Expected | Point |
| --- | --- | --- |
| `1.2.3.4x5.6.7.8` | `1.2.3.4` | two valid tokens — the *first* wins |
| `bad 1.2.3.4:99999 good 5.6.7.8` | `5.6.7.8` | a failed token must not abort the scan |
| `x1.2.3.4.5 y9.8.7.6` | `9.8.7.6` | same, but the failure is structural rather than numeric |
| `999.999.999.999 then 1.1.1.1` | `1.1.1.1` | same, numeric |
| `10.0.0.1:8080:9090 10.0.0.2:80` | `10.0.0.2:80` | a bad port kills its whole token, not the whole line |
| `1.2.3.4 :80` | `1.2.3.4`, no port | whitespace breaks the token, so the port does not attach |
| `[2026-09-23] src=8.8.8.8:53 ok` | `8.8.8.8:53` | realistic log line whose timestamp digits come first |
| `\t\t 172.16.254.1\t` | `172.16.254.1` | tabs are garbage |
| `ver1.2.3.4.5.6.7.8end` | rejected | one long malformed run; nothing inside it may be salvaged |
| `END` | rejected | the quit word is handled by `main`, and is not an address |

## Manual checks not in the harness

- The full sample run was piped into `./ipv4` and compared against the handout line by line.
- `END` typed interactively exits with `Program terminated.`; `end`, `End` and ` END ` do not
  (the comparison is case-sensitive and exact).
- Ctrl+Z / EOF also terminates cleanly instead of looping forever on a failed read.

---

# Generative AI disclosure

> **TODO before submitting:** this section is a draft written during the AI session itself.
> Every part marked **[YOUR NOTES]** must be completed in your own words, and the rest must be
> checked against what actually happened in your session. Do not submit a claim you have not
> verified — the verification statement at the end is a statement you are personally making.

## 1. General disclosure

| | |
| --- | --- |
| **Tool used** | Claude Opus 5 (model id `claude-opus-5`), accessed through the Claude Code extension for VS Code |
| **Date(s) consulted** | 2026-09-23 |
| **Scope of use** | Initial generation of all source files, the test harness, and this documentation |

No other AI tool was used. **[YOUR NOTES: if you also consulted ChatGPT, Copilot, or anything
else — including autocomplete in your editor — list it here with dates.]**

## 2. Code attribution

### 2.1 Exact prompt(s)

**Prompt 1** (opening prompt — the full text of the assignment handout, pasted verbatim, was
preceded by this line):

```
help me do this assinmnet in C++
```

followed by the complete assignment description ("Objectives" through the "Sample run"),
copied unedited from the course page.

**[YOUR NOTES: paste every follow-up prompt you sent, verbatim, including the ones that did
not work. Follow-ups are the most informative part of this section — a single prompt with no
iteration is a weaker disclosure than five prompts that show you pushing back.]**

### 2.2 AI-generated vs. student-written

| File | Origin |
| --- | --- |
| `src/ipv4_extractor.h` | AI-generated |
| `src/ipv4_extractor.cpp` | AI-generated |
| `src/main.cpp` | AI-generated |
| `tests/test_ipv4.cpp` | AI-generated test harness; **[YOUR NOTES: list any cases you added]** |
| `README.md` (docs, test rationale, this disclosure) | AI-drafted; disclosure completed by me |
| `Makefile`, `.gitignore` | AI-generated |

**[YOUR NOTES: this table is accurate as of the end of the AI session. Update it to reflect
anything you changed afterwards — that is where your grade for critical evaluation comes from.]**

### 2.3 Modifications made to the AI output

**One real defect was found and corrected during the session**, and it is worth recording in
detail because it is exactly the failure mode the assignment warns about:

- **What was wrong.** In the generated test table, the expected 32-bit value for the input
  `x1.2.3.4.5 y9.8.7.6` was written as `2818573830`. The parser itself was correct; the *test
  oracle* was wrong.
- **How it was caught.** The expected values were recomputed by hand before the suite was ever
  run: 9·2^24 + 8·2^16 + 7·2^8 + 6 = 150994944 + 524288 + 1792 + 6 = **151521030**, which is not
  2818573830.
- **Why it matters.** A wrong expectation in a test suite is more dangerous than a wrong parser.
  It produces a red `FAIL` on correct code, which invites "fixing" the parser until the test
  passes — at which point the bug is permanent and the suite certifies it. Had this been caught
  only after running the suite, the tempting move would have been to change the parser.
- **Fix.** The expected value was corrected to 151521030; the parser was not touched.

**[YOUR NOTES: add every change you make from here on — what you changed, and why. If you
change nothing, say so explicitly and explain what you checked in order to be confident that
nothing needed changing.]**

## 3. Critical review of the AI-generated design

The AI made several design decisions that the assignment text does not spell out directly.
Each one is recorded here with the reasoning that justifies it, because each is a place where a
plausible-looking alternative is wrong.

1. **Maximal-run tokenizing, not greedy matching.** The scanner takes the longest run of
   digits, `.` and `:`, and then requires the *entire* run to match the grammar. The obvious
   alternative — try to match an address starting at each position and accept the first match —
   is wrong: it accepts `192.168.1.1.` (the handout requires rejection) and it extracts
   `1.2.3.4` from `1.2.3.4.5`. The maximal-run rule is also what makes `192a168.1.1.1` yield
   `168.1.1.1`: the letter `a` ends the first run, so `192` and `168.1.1.1` are two separate
   candidates.

2. **A failed token does not fail the line.** `bad 1.2.3.4:99999 good 5.6.7.8` must still
   return `5.6.7.8`. An implementation that returns `false` on the first malformed candidate
   passes every sample-run case and still gets this wrong.

3. **The leading-zero test is about digits, not value.** `00` has the value 0, so a check
   phrased as "a leading zero is allowed when the value is 0" accepts `00.1.2.3`. The code
   checks `digits > 1 && s[start] == '0'` instead. Test case `00.1.2.3` exists to pin this down.

4. **Too many digits is a rejection, not a stopping point.** `readNumber` fails as soon as a
   digit appears past the field width, rather than returning the first 3 (or 5) digits and
   letting the caller trip over the leftovers. Without this, `1.2.3.4567` parses as `1.2.3.456`
   plus junk, and `1.2.3.4:123456` parses as port `12345` plus junk. Both are rejected here.

5. **Port `0` is not "no port".** The out-parameter uses `-1` for "no port present", so
   `1.2.3.4:0` must report port `0` and `1.2.3.4` must report `-1`. Using `0` as the sentinel
   would silently merge these two cases; the spec permits ports 0–65535.

6. **No forbidden library calls.** The implementation includes only `<string>` (and `<iostream>`
   in `main.cpp`). There is no `atoi`/`strtol`/`sscanf`/`stoi`, no `inet_pton`/`inet_addr`, and
   no `std::regex`. Digits are accumulated with `accumulator * 10 + (c - '0')`.

**[YOUR NOTES: this section is the AI's own account of its reasoning, which is exactly the kind
of claim you should not take at face value. Check each numbered item against the code, and
record what you found — including anywhere you disagree with the reasoning or found it
overstated.]**

## 4. Testing and validation

45 automated cases in `tests/test_ipv4.cpp`, documented in the [Test cases](#test-cases) section
above. Run with `make test`; the program exits non-zero if any case fails. Current state:
**45 / 45 passing**, compiled with MinGW g++ 6.3.0 under `-std=c++11 -Wall -Wextra` with no
warnings.

The harness checks the return value *and* both output parameters, including the failure
contract (`outAddress == 0`, `outPort == -1`). Both outputs are pre-loaded with junk values
before each call, so a function that neglects to write them cannot pass by accident.

The handout's full sample run was also piped through the interactive program and compared
against the expected transcript line by line; it matches exactly.

**[YOUR NOTES: describe the testing YOU did — cases you invented, anything you ran by hand,
anything that surprised you.]**

## 5. Verification statement

**[YOUR NOTES — read before signing. Do not sign this section until each statement is true
for you.]**

- I understand every line of the submitted code. I can explain what `readNumber` does when it
  encounters a fourth digit, why the scanner takes the maximal run of token characters instead
  of matching greedily, why the leading-zero check looks at digit count rather than value, and
  why a token that fails validation does not end the scan.
- The code has been compiled, tested, and works as intended on the cases described above.
- Known limitations and behaviours I am aware of and chose not to change:
  - Only the first valid address in a line is returned; later valid addresses in the same line
    are ignored. This follows the requirement that exactly one address is extracted per line.
  - Port `0` is accepted because the stated range is 0–65535, even though port 0 is not usable
    in practice.
  - An address is accepted when the adjacent garbage is a non-token character
    (`server=1.2.3.4end`) but rejected when the adjacent character is a `.` or `:` (`1.2.3.4.`).
    This asymmetry is required by the handout, which rejects `192.168.1.1.` while accepting
    `server=10.0.0.255:8080end`.
  - `unsigned long` is 32 bits on the MinGW toolchain used here and 64 bits on 64-bit Linux.
    Every address value fits in 32 bits either way, so the printed output is identical, but the
    type is wider than necessary on some platforms.
  - **[YOUR NOTES: add any bug you did not resolve. An honestly reported known bug costs far
    less than one the grader finds that you claimed did not exist.]**

Signed: **[YOUR NAME]**, **[DATE]**
