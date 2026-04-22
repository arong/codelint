// Test for brace initialization style transformations
// Focus: = → {} conversion, = {} → {} removal, auto brace → equals

// 1. EQUALS TO BRACE INITIALIZATION
int a = 10;              // Should suggest: int a{10}
double b = 3.14;         // Should suggest: double b{3.14}
std::string s = "hello"; // Should suggest: std::string s{"hello"}

// 2. EQUALS BRACE TO DIRECT BRACE
int c = {1}; // Should suggest: int c{1}
int d = {};  // Should suggest: int d{}

// 3. AUTO BRACE TO EQUALS (opposite direction)
auto x{42};         // Should suggest: auto x = 42
auto* p{&a};        // Should suggest: auto *p = &a
const auto* cp{&a}; // Should suggest: const auto *cp = &a

// 4. UNSIGNED SUFFIX
unsigned u = 100;  // Should suggest: unsigned u{100U}
uint64_t big = 42; // Should suggest: uint64_t big{42UL}

// 5. CALL INIT TO BRACE
std::string str("world"); // Should suggest: std::string str{"world"}

// 6. VALID CODE (no warnings)
int valid{10};         // Already correct brace init
auto valid2 = 42;      // Already correct auto equals
unsigned valid3{100U}; // Already has suffix
