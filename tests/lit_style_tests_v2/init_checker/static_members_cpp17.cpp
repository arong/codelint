// RUN: %check_codelint %s codelint-init %t

// C++17 feature: static inline data members
// static inline allows definition at point of declaration (C++17+)
// These should NOT trigger warnings — they are static data members.

class WithInlineStatic {
  static inline int count = 0;
  static inline const int kMaxSize = 100;
  static inline int unchecked_count;
  double val;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:10: warning: member variable 'val' is not initialized in
  // constructor [codelint-init]

public:
  WithInlineStatic() {
  }
};

class WithConstexprStatic {
  constexpr static int kSize = 64;
  constexpr static unsigned kMask = 0xFF;
  int data;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: warning: member variable 'data' is not initialized in
  // constructor [codelint-init]

public:
  WithConstexprStatic() {
  }
};

class MixedInlineAndRegular {
  static inline int x = 10;
  static const int kA;
  static inline double ratio = 1.5;
  int y;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: warning: member variable 'y' is not initialized in
  // constructor [codelint-init]
  int z;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: warning: member variable 'z' is not initialized in
  // constructor [codelint-init]

public:
  MixedInlineAndRegular() {
  }
};

// === Expected Fixed Output ===
// CHECK-FIXES: class WithInlineStatic {
// CHECK-FIXES:   static inline int count = 0;
// CHECK-FIXES:   static inline const int kMaxSize = 100;
// CHECK-FIXES:   static inline int unchecked_count;
// CHECK-FIXES:   double val{};
// CHECK-FIXES: public:
// CHECK-FIXES:   WithInlineStatic() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: class WithConstexprStatic {
// CHECK-FIXES:   constexpr static int kSize = 64;
// CHECK-FIXES:   constexpr static unsigned kMask = 0xFF;
// CHECK-FIXES:   int data{};
// CHECK-FIXES: public:
// CHECK-FIXES:   WithConstexprStatic() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: class MixedInlineAndRegular {
// CHECK-FIXES:   static inline int x = 10;
// CHECK-FIXES:   static const int kA;
// CHECK-FIXES:   static inline double ratio = 1.5;
// CHECK-FIXES:   int y{};
// CHECK-FIXES:   int z{};
// CHECK-FIXES: public:
// CHECK-FIXES:   MixedInlineAndRegular() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
