// Test fixture for function call detection - pointer and reference parameter passing
// Variables passed to functions via pointer/reference should NOT suggest const
// because the function may modify them

// Function that takes pointer parameter - may modify target
void modify_pointer(int* p);

// Function that takes reference parameter - may modify target
void modify_reference(int& r);

// External function declaration - assume may modify
extern void external_func(int* p);

// Test case 1: Variable passed to pointer parameter
void test_pointer() {
  int x = 10;
  modify_pointer(&x);
}

// Test case 2: Variable passed to reference parameter
void test_reference() {
  int y = 20;
  modify_reference(y);
}

// Test case 3: External function with pointer parameter
void test_external() {
  int z = 30;
  external_func(&z);
}

// Test case 4: Multiple variables passed to pointer function
void test_multiple() {
  int a = 1;
  int b = 2;
  int c = 3;
  modify_pointer(&a);
  modify_pointer(&b);
  modify_pointer(&c);
}

// Test case 5: Reference in different function
void test_ref_params() {
  int val1 = 100;
  int val2 = 200;
  modify_reference(val1);
  modify_reference(val2);
}

// Test case 6: Mix of pointer and reference calls
void test_mixed() {
  int p_val = 50;
  int r_val = 60;
  modify_pointer(&p_val);
  modify_reference(r_val);
}

// Test case 7: Global variables passed to functions
int global_x;
int global_y;

void test_globals() {
  modify_pointer(&global_x);
  modify_reference(global_y);
}

// Test case 8: Different types passed to pointer function
void test_types() {
  int s = 5;
  int l = 10;
  int i32 = 15;
  int ui64 = 20;
  modify_pointer(&s);
  modify_pointer(&l);
  modify_pointer(&i32);
  modify_pointer(&ui64);
}

// Test case 9: Const pointer parameter - should NOT add const to variable
void modify_const_ptr(const int* p);

// Test case 10: Const reference parameter - should NOT add const to variable
void modify_const_ref(const int& r);

void test_const_params() {
  int cv1 = 1;
  int cv2 = 2;
  modify_const_ptr(&cv1);
  modify_const_ref(cv2);
}

int main() {
  test_pointer();
  test_reference();
  test_external();
  test_multiple();
  test_ref_params();
  test_mixed();
  test_globals();
  test_types();
  test_const_params();
  return 0;
}
