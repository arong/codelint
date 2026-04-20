// Test: Exception safety and error handling in lifecycle
// Tests exception handling, error codes, and cleanup patterns

#include <cstddef>
#include <stdexcept>

// ========================================
// Pattern: RAII Wrapper with Manual Fallback
// ========================================

class SafeResource {
public:
  int* data_;
  bool owns_data_;

  SafeResource() : data_(nullptr), owns_data_(false) {
  }

  void Acquire() {
    if (data_ != nullptr) {
      return;
    }
    data_ = new int(42);
    owns_data_ = true;
  }

  void Release() {
    if (owns_data_ && data_ != nullptr) {
      delete data_;
    }
    data_ = nullptr;
    owns_data_ = false;
  }

  void UseUnsafe() {
    *data_ = 100;
  }

  void UseSafe() {
    if (data_ != nullptr) {
      *data_ = 100;
    }
  }
};

void testSafeResourceNormal() {
  SafeResource res;
  res.Acquire();
  res.UseSafe();
  res.Release();
}

void testSafeResourceUseBeforeAcquire() {
  SafeResource res;
  res.UseUnsafe();
}

void testSafeResourceUseAfterRelease() {
  SafeResource res;
  res.Acquire();
  res.Release();
  res.UseUnsafe();
}

// ========================================
// Pattern: Error Code Pattern
// ========================================

class ErrorCodeResource {
public:
  int* buffer_;
  int error_code_;

  ErrorCodeResource() : buffer_(nullptr), error_code_(0) {
  }

  bool Initialize(size_t size) {
    if (buffer_ != nullptr) {
      error_code_ = -1;
      return false;
    }
    buffer_ = new int[size];
    error_code_ = 0;
    return true;
  }

  bool ProcessUnsafe() {
    buffer_[0] = 1;
    return true;
  }

  bool ProcessSafe() {
    if (buffer_ == nullptr) {
      error_code_ = -2;
      return false;
    }
    buffer_[0] = 1;
    return true;
  }

  void Cleanup() {
    delete[] buffer_;
    buffer_ = nullptr;
    error_code_ = 0;
  }
};

void testErrorCodeNormal() {
  ErrorCodeResource res;
  if (res.Initialize(100)) {
    res.ProcessSafe();
  }
  res.Cleanup();
}

void testErrorCodeProcessBeforeInit() {
  ErrorCodeResource res;
  res.ProcessUnsafe();
}

// ========================================
// Pattern: Factory with Lazy Creation
// ========================================

class FactoryCreated {
public:
  int* internal_data_;

  FactoryCreated() : internal_data_(nullptr) {
  }

  static FactoryCreated* Create() {
    FactoryCreated* obj = new FactoryCreated();
    obj->internal_data_ = new int(100);
    return obj;
  }

  void Destroy() {
    delete internal_data_;
    internal_data_ = nullptr;
  }

  void Access() {
    *internal_data_ = 200;
  }
};

void testFactoryNormal() {
  FactoryCreated* obj = FactoryCreated::Create();
  obj->Access();
  obj->Destroy();
  delete obj;
}

void testFactoryAfterDestroy() {
  FactoryCreated* obj = FactoryCreated::Create();
  obj->Destroy();
  obj->Access();
  delete obj;
}

// ========================================
// Pattern: Move Semantics with Null
// ========================================

class MovableResource {
public:
  int* data_;

  MovableResource() : data_(nullptr) {
  }

  explicit MovableResource(int value) : data_(new int(value)) {
  }

  MovableResource(MovableResource&& other) noexcept : data_(other.data_) {
    other.data_ = nullptr;
  }

  MovableResource& operator=(MovableResource&& other) noexcept {
    if (this != &other) {
      delete data_;
      data_ = other.data_;
      other.data_ = nullptr;
    }
    return *this;
  }

  ~MovableResource() {
    delete data_;
  }

  void Access() {
    *data_ = 999;
  }
};

void testMoveNormal() {
  MovableResource res1(42);
  MovableResource res2 = std::move(res1);
  res2.Access();
}

void testMoveAfterMove() {
  MovableResource res1(42);
  MovableResource res2 = std::move(res1);
  res1.Access();
}

// ========================================
// Pattern: Optional Pattern Simulation
// ========================================

class OptionalLike {
public:
  int* value_;
  bool has_value_;

  OptionalLike() : value_(nullptr), has_value_(false) {
  }

  void SetValue(int v) {
    if (value_ == nullptr) {
      value_ = new int(v);
    } else {
      *value_ = v;
    }
    has_value_ = true;
  }

  void Clear() {
    delete value_;
    value_ = nullptr;
    has_value_ = false;
  }

  int GetValueUnsafe() {
    return *value_;
  }

  int GetValueSafe() {
    if (has_value_ && value_ != nullptr) {
      return *value_;
    }
    return 0;
  }
};

void testOptionalNormal() {
  OptionalLike opt;
  opt.SetValue(42);
  int v = opt.GetValueSafe();
}

void testOptionalGetBeforeSet() {
  OptionalLike opt;
  int v = opt.GetValueUnsafe();
}

void testOptionalGetAfterClear() {
  OptionalLike opt;
  opt.SetValue(42);
  opt.Clear();
  int v = opt.GetValueUnsafe();
}

// ========================================
// Pattern: Observer Pattern with Weak Reference
// ========================================

class Subject {
public:
  int* state_;

  Subject() : state_(nullptr) {
  }

  void Initialize() {
    state_ = new int(0);
  }

  void SetState(int s) {
    *state_ = s;
  }

  int GetState() {
    return *state_;
  }

  void Terminate() {
    delete state_;
    state_ = nullptr;
  }
};

class Observer {
public:
  Subject* subject_;

  Observer() : subject_(nullptr) {
  }

  void Attach(Subject* s) {
    subject_ = s;
  }

  void OnUpdateUnsafe() {
    int state = subject_->GetState();
  }

  void OnUpdateSafe() {
    if (subject_ != nullptr && subject_->state_ != nullptr) {
      int state = subject_->GetState();
    }
  }
};

void testObserverNormal() {
  Subject sub;
  sub.Initialize();
  Observer obs;
  obs.Attach(&sub);
  obs.OnUpdateSafe();
  sub.Terminate();
}

void testObserverBeforeAttach() {
  Observer obs;
  obs.OnUpdateUnsafe();
}

void testObserverAfterSubjectTerminate() {
  Subject sub;
  sub.Initialize();
  Observer obs;
  obs.Attach(&sub);
  sub.Terminate();
  obs.OnUpdateUnsafe();
}
