// RUN: %codelint %s codelint-init %t
// Test for bitfield member handling
// P1-1: Bitfield members should NOT be suggested to add {} after the name

struct Flags {
  int valid : 1;
  int count : 7;
  int reserved : 8;
};

struct BitfieldWithInit {
  int flag : 1 = 0;
  int value : 4 = 5;
};

struct MixedBitfield {
  int normal_member;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  int bitfield : 4;
  double regular;
  // CHECK-MESSAGES: :[@LINE-1]:10: error: field is not initialized  [codelint-init]
};

class BitfieldClass {
  int status : 4;
  int error : 1;
  int reserved : 3;

public:
  BitfieldClass() {
  }
};

// === Expected Fixed Output ===
// CHECK-FIXES: struct Flags {
// CHECK-FIXES:   int valid : 1;
// CHECK-FIXES:   int count : 7;
// CHECK-FIXES:   int reserved : 8;
// CHECK-FIXES: };
// CHECK-FIXES: struct BitfieldWithInit {
// CHECK-FIXES:   int flag : 1 = 0;
// CHECK-FIXES:   int value : 4 = 5;
// CHECK-FIXES: };
// CHECK-FIXES: struct MixedBitfield {
// CHECK-FIXES:   int normal_member{};
// CHECK-FIXES:   int bitfield : 4;
// CHECK-FIXES:   double regular{};
// CHECK-FIXES: };
