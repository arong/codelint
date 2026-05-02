// RUN: %codelint %s codelint-function-size %t

void small_function() { // OK - only 1 line
  int x = 1;
}

void big_function() { // VIOLATES: has 10 effective lines (>= 50)
  int a1 = 1;
  int a2 = 2;
  int a3 = 3;
  int a4 = 4;
  int a5 = 5;
  int a6 = 6;
  int a7 = 7;
  int a8 = 8;
  int a9 = 9;
  int a10 = 10;
  int b1 = 1;
  int b2 = 2;
  int b3 = 3;
  int b4 = 4;
  int b5 = 5;
  int b6 = 6;
  int b7 = 7;
  int b8 = 8;
  int b9 = 9;
  int b10 = 10;
  int c1 = 1;
  int c2 = 2;
  int c3 = 3;
  int c4 = 4;
  int c5 = 5;
  int c6 = 6;
  int c7 = 7;
  int c8 = 8;
  int c9 = 9;
  int c10 = 10;
  int d1 = 1;
  int d2 = 2;
  int d3 = 3;
  int d4 = 4;
  int d5 = 5;
  int d6 = 6;
  int d7 = 7;
  int d8 = 8;
  int d9 = 9;
  int d10 = 10;
  int e1 = 1;
  int e2 = 2;
  int e3 = 3;
  int e4 = 4;
  int e5 = 5;
  // CHECK-MESSAGES: :[[@LINE-10]]:6: warning: function 'big_function' has 50 lines (exceeds 50 line
  // limit)  [codelint-function-size]
}
