// RUN: %check_codelint %s codelint-init %t
// Test for std container type initialization
// Focus: vector, map, set, array, deque, pair, tuple, unordered_map

#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

// 1. SEQUENCE CONTAINERS
std::vector<int> vec1;
// CHECK-MESSAGES: :[[@LINE-1]]:18: warning: variable is not explicitly initialized  [codelint-init]
std::deque<int> deque1;
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: variable is not explicitly initialized  [codelint-init]
std::array<int, 5> arr1;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: variable is not initialized  [codelint-init]

// 2. ASSOCIATIVE CONTAINERS
std::map<int, int> map1;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: variable is not explicitly initialized  [codelint-init]
std::set<int> set1;
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: variable is not explicitly initialized  [codelint-init]
std::unordered_map<int, int> umap1;
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: variable is not explicitly initialized  [codelint-init]

// 3. TUPLE TYPES
std::pair<int, int> pair1;
// CHECK-MESSAGES: :[[@LINE-1]]:21: warning: variable is not explicitly initialized  [codelint-init]
std::tuple<int, double> tpl1;
// CHECK-MESSAGES: :[[@LINE-1]]:25: warning: variable is not explicitly initialized  [codelint-init]

void test_local_containers() {
  std::vector<int> local_vec;
  // CHECK-MESSAGES: :[[@LINE-1]]:20: warning: variable is not explicitly initialized
  // [codelint-init]

  std::map<int, int> local_map;
  // CHECK-MESSAGES: :[[@LINE-1]]:22: warning: variable is not explicitly initialized
  // [codelint-init]

  std::vector<uint8_t> vec = {1, 2, 3};

  std::vector<std::vector<uint8_t>> island = {
      {1, 2, 3},
      {4, 5, 6},
      {7, 8, 9},
  };
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <array>
// CHECK-FIXES: #include <cstdint>
// CHECK-FIXES: #include <deque>
// CHECK-FIXES: #include <map>
// CHECK-FIXES: #include <set>
// CHECK-FIXES: #include <tuple>
// CHECK-FIXES: #include <unordered_map>
// CHECK-FIXES: #include <utility>
// CHECK-FIXES: #include <vector>
// CHECK-FIXES: std::vector<int> vec1{};
// CHECK-FIXES: std::deque<int> deque1{};
// CHECK-FIXES: std::array<int, 5> arr1{};
// CHECK-FIXES: std::map<int, int> map1{};
