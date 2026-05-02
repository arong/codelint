// RUN: %check_codelint %s codelint-init %t
// Test for template classes with initializer_list constructors

#include <initializer_list>
#include <string>
#include <vector>

template <typename T> class TemplateContainer {
public:
  TemplateContainer(std::initializer_list<T> list) {
  }
  TemplateContainer(size_t count, const T& value) {
  }
  TemplateContainer() {
  }
  TemplateContainer(T value) {
  }
};

void test_template_container() {
  TemplateContainer<int> tc1(5, 0);
  TemplateContainer<int> tc2(3, 10);
  TemplateContainer<int> tc3{1, 2, 3};
  TemplateContainer<int> tc4{};
  TemplateContainer<int> tc5;
  // CHECK-MESSAGES: :[[@LINE-1]]:26: warning: variable is not explicitly initialized
  // [codelint-init]
  TemplateContainer<int> tc6 = 6;
}

template <typename T> class OnlyInitList {
public:
  OnlyInitList(std::initializer_list<T> list) {
  }
};

void test_only_init_list() {
  OnlyInitList<int> ol1{1, 2, 3};
  OnlyInitList<int> ol2{};
}

template <typename T> class NoInitList {
public:
  NoInitList(size_t count) {
  }
  NoInitList(const T& value) {
  }
};

void test_no_init_list() {
  NoInitList<int> nl1(5);
  NoInitList<int> nl2 = 10;
}

void test_std_templates() {
  std::vector<int> vec1(5, 0);
  std::vector<std::string> vec2(3, "test");
  std::string str1(5, 'a');
  std::vector<int> vec3 = {1, 2, 3};
  std::string str2 = "hello";
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <initializer_list>
// CHECK-FIXES: #include <string>
// CHECK-FIXES: #include <vector>
// CHECK-FIXES: template <typename T> class TemplateContainer {
// CHECK-FIXES: public:
// CHECK-FIXES:   TemplateContainer(std::initializer_list<T> list) {
// CHECK-FIXES:   }
// CHECK-FIXES:   TemplateContainer(size_t count, const T& value) {
// CHECK-FIXES:   }
// CHECK-FIXES:   TemplateContainer() {
// CHECK-FIXES:   }
// CHECK-FIXES:   TemplateContainer(T value) {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: void test_template_container() {
// CHECK-FIXES:   TemplateContainer<int> tc1(5, 0);
