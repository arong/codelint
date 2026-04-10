// Test for pointer type initialization
// Focus: raw pointers, pointer arrays, pointer-to-member

// 1. BASIC POINTER TYPES
int* ptr1{};
const int* ptr2{};
int** ptr3{};
void* void_ptr{};

// 2. ARRAY OF POINTERS
int* ptr_array1[10]{};
const char* str_ptr_array[5]{};

// 3. POINTER TO MEMBER
class ClassForMemberPtr {
public:
  int member{};
};
int ClassForMemberPtr::* pmem{};

void test_local_pointers() {
  int* local_ptr{};
  double* local_dptr{};
  int** local_pptr{};
}