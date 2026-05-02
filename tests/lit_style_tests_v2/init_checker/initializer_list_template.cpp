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
  TemplateContainer<int> tc7{7};
  // CHECK-MESSAGES: :[[@LINE-1]]:26: warning: brace initialization with single element calls
  // initializer_list constructor; consider using direct initialization '()' to call the
  // single-argument constructor [codelint-init]
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

// Test: template class with ONLY initializer_list constructor (no single-arg ctor)
// Should NOT warn when using {value} because there's no alternative constructor
template <typename T> class OnlyInitListNoSingle {
public:
  OnlyInitListNoSingle(std::initializer_list<T> list) {
  }
  OnlyInitListNoSingle() {
  }
};

void test_only_init_list_no_single() {
  OnlyInitListNoSingle<int> obj1{5};
  OnlyInitListNoSingle<int> obj2{1, 2};
  OnlyInitListNoSingle<int> obj3{};
}

// Test: concrete class with ONLY initializer_list constructor (no single-arg ctor)
// Should NOT warn when using {value}
class ConcreteOnlyInitList {
public:
  ConcreteOnlyInitList(std::initializer_list<int> list) {
  }
  ConcreteOnlyInitList() {
  }
};

void test_concrete_only_init_list() {
  ConcreteOnlyInitList obj1{5};
  ConcreteOnlyInitList obj2{1, 2};
  ConcreteOnlyInitList obj3{};
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
// CHECK-FIXES:   TemplateContainer<int> tc2(3, 10);
// CHECK-FIXES:   TemplateContainer<int> tc3{1, 2, 3};
// CHECK-FIXES:   TemplateContainer<int> tc4{};
// CHECK-FIXES:   TemplateContainer<int> tc5{};
// CHECK-FIXES:   TemplateContainer<int> tc6 = 6;
// CHECK-FIXES:   TemplateContainer<int> tc7{7};
// CHECK-FIXES: }
// CHECK-FIXES: template <typename T> class OnlyInitList {
// CHECK-FIXES: public:
// CHECK-FIXES:   OnlyInitList(std::initializer_list<T> list) {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: void test_only_init_list() {
// CHECK-FIXES:   OnlyInitList<int> ol1{1, 2, 3};
// CHECK-FIXES:   OnlyInitList<int> ol2{};
// CHECK-FIXES: }
// CHECK-FIXES: template <typename T> class NoInitList {
// CHECK-FIXES: public:
// CHECK-FIXES:   NoInitList(size_t count) {
// CHECK-FIXES:   }
// CHECK-FIXES:   NoInitList(const T& value) {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: void test_no_init_list() {
// CHECK-FIXES:   NoInitList<int> nl1(5);
// CHECK-FIXES:   NoInitList<int> nl2 = 10;
// CHECK-FIXES: }
// CHECK-FIXES: void test_std_templates() {
// CHECK-FIXES:   std::vector<int> vec1(5, 0);
// CHECK-FIXES:   std::vector<std::string> vec2(3, "test");
// CHECK-FIXES:   std::string str1(5, 'a');
// CHECK-FIXES:   std::vector<int> vec3 = {1, 2, 3};
// CHECK-FIXES:   std::string str2 = "hello";
// CHECK-FIXES: }
// CHECK-FIXES: template <typename T>
// CHECK-FIXES: class OnlyInitListNoSingle {
// CHECK-FIXES: public:
// CHECK-FIXES:   OnlyInitListNoSingle(std::initializer_list<T> list) {
// CHECK-FIXES:   }
// CHECK-FIXES:   OnlyInitListNoSingle() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: void test_only_init_list_no_single() {
// CHECK-FIXES:   OnlyInitListNoSingle<int> obj1{5};
// CHECK-FIXES:   OnlyInitListNoSingle<int> obj2{1, 2};
// CHECK-FIXES:   OnlyInitListNoSingle<int> obj3{};
// CHECK-FIXES: }
// CHECK-FIXES: class ConcreteOnlyInitList {
// CHECK-FIXES: public:
// CHECK-FIXES:   ConcreteOnlyInitList(std::initializer_list<int> list) {
// CHECK-FIXES:   }
// CHECK-FIXES:   ConcreteOnlyInitList() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: void test_concrete_only_init_list() {
// CHECK-FIXES:   ConcreteOnlyInitList obj1{5};
// CHECK-FIXES:   ConcreteOnlyInitList obj2{1, 2};
// CHECK-FIXES:   ConcreteOnlyInitList obj3{};
// CHECK-FIXES: }
