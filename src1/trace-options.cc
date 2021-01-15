/**
 * File: trace-options.cc
 * ----------------------
 * Presents the implementation of the one function exported by trace-options.h
 */

#include "trace-options.h"
#include <string>
#include <string.h>
// #include "string-utils.h"
using namespace std;

// Implement startwWith due to lack of string-utils.h
bool startsWith(const char *s1, const char *s2) {
  size_t s1len = strlen(s1);
  size_t s2len = strlen(s2);
  return s1len < s2len ? false : strncmp(s1, s2, s2len) == 0;
}

static const string kSimpleFlag = "--simple";
static const string kRebuildFlag = "--rebuild";
size_t processCommandLineFlags(bool& simple, bool& rebuild, char *argv[]) throw (TraceException) {  
  size_t numFlags = 0;
  for (int i = 1; argv[i] != NULL && startsWith(argv[i], "--"); i++) {
    if (argv[i] == kSimpleFlag) simple = true;
    else if (argv[i] == kRebuildFlag) rebuild = true;
    else throw TraceException(string(argv[0]) + ": Unrecognized flag (" + argv[i] + " )");
    numFlags++;
  }
  
  return numFlags;
}
