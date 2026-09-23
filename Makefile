CXX      := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -Isrc

# g++ appends .exe on Windows; naming the targets accurately keeps make from
# rebuilding everything on every invocation.
EXE :=
ifeq ($(OS),Windows_NT)
EXE := .exe
endif

all: ipv4$(EXE) tests/run_tests$(EXE)

ipv4$(EXE): src/main.cpp src/ipv4_extractor.cpp src/ipv4_extractor.h
	$(CXX) $(CXXFLAGS) src/main.cpp src/ipv4_extractor.cpp -o ipv4

tests/run_tests$(EXE): tests/test_ipv4.cpp src/ipv4_extractor.cpp src/ipv4_extractor.h
	$(CXX) $(CXXFLAGS) tests/test_ipv4.cpp src/ipv4_extractor.cpp -o tests/run_tests

test: tests/run_tests$(EXE)
	./tests/run_tests

clean:
	-rm -f ipv4 ipv4.exe tests/run_tests tests/run_tests.exe

.PHONY: all test clean
