// RUN: %check_codelint %s codelint-singleton %t

// Return Pointer - NOT a singleton
class Resource {
public:
  static Resource* get() {
    static Resource res;
    return &res;
  }
};

// Return by Value - NOT a singleton
class Factory {
public:
  static Factory create() {
    return Factory();
  }
};

// Parameter Reference - NOT a singleton
int& getRef(int& x) {
  return x;
}

// Static Local Variable - NOT a singleton
void helper() {
  static int counter = 0;
  counter++;
}
