// multi_issue.cpp - Edge case fixture for multiple issues on same line
// Contains multiple codelint issues on a single line for testing multi-issue reporting
// Expected: codelint should report multiple issues on line 9

#include <iostream>

int main() {
  int a = 1;
  unsigned int b = 2;
  int c = 3;
  return 0;
}
