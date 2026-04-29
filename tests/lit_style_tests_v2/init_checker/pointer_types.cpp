// RUN: %codelint %s codelint-init %t
// Test for pointer type initialization
// Focus: raw pointers, pointer arrays, pointer-to-member

// 1. BASIC POINTER TYPES
int* ptr1;
// CHECK-MESSAGES: :6:6: error: variable is not initialized  [codelint-init]
const int* ptr2;
// CHECK-MESSAGES: :8:12: error: variable is not initialized  [codelint-init]
int** ptr3;
// CHECK-MESSAGES: :10:7: error: variable is not initialized  [codelint-init]
void* void_ptr;
// CHECK-MESSAGES: :12:7: error: variable is not initialized  [codelint-init]

// 2. ARRAY OF POINTERS
int* ptr_array1[10];
// CHECK-MESSAGES: :16:6: error: C-style array is not initialized  [codelint-init]
const char* str_ptr_array[5];
// CHECK-MESSAGES: :18:13: error: C-style array is not initialized  [codelint-init]

// 3. POINTER TO MEMBER
class ClassForMemberPtr {
public:
  int member;
// CHECK-MESSAGES: :24:7: error: field is not initialized  [codelint-init]
};
int ClassForMemberPtr::* pmem;
// CHECK-MESSAGES: :27:26: error: variable is not initialized  [codelint-init]

void test_local_pointers() {
  int* local_ptr;
// CHECK-MESSAGES: :31:8: error: variable is not initialized  [codelint-init]
  double* local_dptr;
// CHECK-MESSAGES: :33:11: error: variable is not initialized  [codelint-init]
  int** local_pptr;
// CHECK-MESSAGES: :35:9: error: variable is not initialized  [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: int* ptr1{};
// CHECK-FIXES: const int* ptr2{};
// CHECK-FIXES: int** ptr3{};
// CHECK-FIXES: void* void_ptr{};
// CHECK-FIXES: int* ptr_array1[10]{};
// CHECK-FIXES: const char* str_ptr_array[5]{};
// CHECK-FIXES: class ClassForMemberPtr {
// CHECK-FIXES: public:
// CHECK-FIXES:   int member{};
// CHECK-FIXES: };
// CHECK-FIXES: int ClassForMemberPtr::* pmem{};
