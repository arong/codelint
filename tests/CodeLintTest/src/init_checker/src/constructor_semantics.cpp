// Test for constructor semantics - cases where = -> {} would change behavior
// CRITICAL: These cases should NOT trigger warnings because brace init changes semantics

#include <deque>
#include <list>
#include <string>
#include <vector>

void test_vector_fill_constructor() {
  std::vector<int> vec1(5, 0);
  std::vector<int> vec2(10, 42);
  std::vector<double> vec3(3, 3.14);
}

void test_string_fill_constructor() {
  std::string str1(5, 'a');
  std::string str2(10, 'x');
}

void test_deque_fill_constructor() {
  std::deque<int> dq1(5, 0);
  std::deque<int> dq2(3, 100);
}

void test_list_fill_constructor() {
  std::list<int> lst1(5, 0);
  std::list<int> lst2(4, 50);
}

void test_iterator_range_constructor() {
  std::vector<int> source{1, 2, 3, 4, 5};

  std::vector<int> vec1{source.begin(), source.end()};
  std::string str1{source.begin(), source.end()};

  std::vector<int> vec2(source.begin(), source.end());
  std::string str2(source.begin(), source.end());
}

void test_safe_to_convert() {
  std::string str1 = "hello";
  std::string str2("world");

  std::vector<int> vec1 = {1, 2, 3};
}

void test_already_brace_init() {
  std::vector<int> vec1{1, 2, 3};
  std::vector<int> vec2{};
  std::string str1{"hello"};
  std::string str2{};
}

void test_mixed_types() {
  std::vector<std::string> vec1(3, "test");

  std::string str1 = "text";
}
