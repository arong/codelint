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
std::deque<int> deque1;
std::array<int, 5> arr1;

// 2. ASSOCIATIVE CONTAINERS
std::map<int, int> map1;
std::set<int> set1;
std::unordered_map<int, int> umap1;

// 3. TUPLE TYPES
std::pair<int, int> pair1;
std::tuple<int, double> tpl1;

void test_local_containers() {
  std::vector<int> local_vec;
  std::map<int, int> local_map;

  std::vector<uint8_t> vec = {1, 2, 3};

  std::vector<std::vector<uint8_t>> island = {
      {1, 2, 3},
      {4, 5, 6},
      {7, 8, 9},
  };
}
