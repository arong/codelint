// Test for type alias initialization
// Focus: using declarations

// 1. SIMPLE TYPE ALIASES
using IntAlias = int;
using DoubleAlias = double;
IntAlias alias1{};
DoubleAlias alias2{};

// 2. MULTIPLE ALIASES FOR SAME TYPE
using IntAlias2 = int;
IntAlias2 alias3{};

// 3. ALIAS IN LOCAL SCOPE
void test_alias_local() {
  using LocalInt = int;
  using LocalDouble = double;
  LocalInt local_alias1{};
  LocalDouble local_alias2{};
}
