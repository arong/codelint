// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test for qualified name tags [Class::Method]

#define LOG(msg) printf(msg)

class MyClass {
public:
  void FuncA() {
    LOG("[FuncA] simple match");
    LOG("[MyClass::FuncA] qualified match");

    LOG("[OtherClass::FuncA] wrong class");
    // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'OtherClass::FuncA' does not match
    // enclosing function 'FuncA'  [codelint-log-tag-mismatch]

    LOG("[MyClass::WrongFunc] wrong method");
    // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'MyClass::WrongFunc' does not match
    // enclosing function 'FuncA'  [codelint-log-tag-mismatch]
  }
};
