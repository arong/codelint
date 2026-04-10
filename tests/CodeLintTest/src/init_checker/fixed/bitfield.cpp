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
  int normal_member{};
  int bitfield : 4;
  double regular{};
};

class BitfieldClass {
  int status : 4;
  int error : 1;
  int reserved : 3;

public:
  BitfieldClass() {
  }
};
