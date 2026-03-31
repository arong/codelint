// Test file for initialization checks - comprehensive test cases
#include <array>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// 1. INTEGER TYPES - 8, 16, 32, 64 bit
char c1{};
unsigned char uc1{};
signed char sc1{};
short s1{};
unsigned short us1{};
int16_t i16{};
uint16_t ui16{};
int i1{};
unsigned int ui1{};
long l1{};
unsigned long ul1{};
int32_t i32{};
uint32_t ui32{};
long long ll1{};
unsigned long long ull1{};
int64_t i64{};
uint64_t ui64{};

// 2. FLOATING POINT TYPES
float f1{};
double d1{};
long double ld1{};

// 3. BOOLEAN
bool b1{};

// 4. STRING TYPES
const char * str1{};
std::string str2;

// 5. POINTER TYPES
int * ptr1{};
const int * ptr2{};
int ** ptr3{};
void * void_ptr{};

// 6. STRUCTURES
struct CStruct {
int x{};
double y{};
};
CStruct cs1;
struct CPPStruct {
int a{};
std::string b{};
float c{};
};
CPPStruct cpps1;

// 7. CLASSES
class TestClass {
 public:
int value{};
std::string name{};
  TestClass() : value(0) {}
};
TestClass tc1;

// 8. ENUMS
enum Color { RED, GREEN, BLUE };
enum class ErrorCode { None, Unknown, Timeout };
Color color1{};
ErrorCode ec1{};

// 9. STD CONTAINERS
std::vector<int> vec1;
std::map<int, std::string> map1;
std::array<int, 5> arr1_array;
std::tuple<int, double> tpl1;

// 10. CONSTANT AND CONSTANTEXPR
const int const_val1{42};
constexpr int constexpr_val1{100};
const double const_pi{3.14159};
int var_can_be_const1{10};
int var_can_be_const2{20};
int var_can_be_const3{30};

// 11. INITIALIZATION STYLES
int c_style1{10};
int c_style2{20};
double c_style3{3.14};
int braced1{10};
int braced2{20};
double braced3{3.14};

// 12. SIGNED/UNSIGNED MIXED
int si1{};
unsigned int uii1{};
short ss1{};
unsigned short uis1{};

// 13. GLOBAL SCOPE VARIABLES
int global_var1{};
int static_global1{};

// 14. FUNCTIONS - LOCAL VARIABLES
void test_function1() {
int local1{};
int local2{};
double local3{};
char local4{};
bool local5{};
  int local6{10};
  int local7{20};
  std::string local8("hello");
}

void test_function2() {
float f_local{};
long lng_local{};
short sh_local{};
}

void test_function3() {
  int a{1};
  int b{2};
  int c{3};
  int d{4};
  int e{5};
}

// 15. COMPLEX SCENARIOS
struct ComplexStruct {
int x{};
double y{};
std::string z{};
int * ptr{};
};
void complex_test() {
  ComplexStruct cs;
  int* raw_array{new int[10]};
  delete[] raw_array;
}

// 16. SPECIFIC TEST CASES FOR LINT
void something() {
  int const_candidate1{100};
  int const_candidate2{200};
  int const_candidate3{300};
  const int already_const1{1000};
  const int already_const2{2000};
  constexpr int const_expr1{3000};
}

void modify_test() {
  int modifiable1{10};
  int modifiable2{20};
  modifiable1++;
  modifiable2 += 5;
}

// 17. TYPE ALIASES
using IntAlias = int;
using DoubleAlias = double;
IntAlias alias1{};
DoubleAlias alias2{};
using IntAlias2 = int;
IntAlias2 alias3{};

// 18. TEMPLATE TYPES
std::pair<int, int> pair1;
std::deque<int> deque1;
std::set<int> set1;
std::unordered_map<int, std::string> umap1;

// 19. MULTI-DIMENSIONAL ARRAYS
int[3][4] arr2d{};
int[2][3][4] arr3d{};

// 20. EXTERNAL AND FORWARD DECLARATIONS
int extern_var{};
int extern_var2{};

// 21. VARIADIC AND SPECIAL TYPES
size_t sz1{};
ptrdiff_t pt1{};
wchar_t wc1{};
char16_t char16_1{};
char32_t char32_1{};

// 22. UNION
union UnionType {
int i{};
double d{};
char c{};
};
UnionType ut1;

// 23. NESTED STRUCTURES
struct Outer {
  struct Inner {
int x{};
int y{};
struct Inner inner{};
int outer_val{};
} outer1;

// 24. VIRTUAL FUNCTIONS
class BaseClass {
 public:
  virtual void foo() {}
int base_val{};
};
class DerivedClass : public BaseClass {
 public:
  void foo() override {}
int derived_val{};
};
DerivedClass dc1;

// 25. ARRAY OF POINTERS
int *[10] ptr_array1{};
const char *[5] str_ptr_array{};

// 26. STRUCT WITH UNION
struct StructWithUnion {
int tag{};
  union {
int ival{};
double dval{};
char * sval{};
union (unnamed union at /home/aronic/playground/cmdtools/cndy/tests/init_check_src.cpp:231:3) u{};
} swu1;

// 27. Template Instantiations
std::vector<std::string> vec_str1;
std::map<std::string, int> map_str_int1;

// 28. Pointer to Member
class ClassForMemberPtr {
 public:
int member{};
};
int ClassForMemberPtr::* pmem{nullptr};

// 29. Multiple Variables on One Line
int x3{};
