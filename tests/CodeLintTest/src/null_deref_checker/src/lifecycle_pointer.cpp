// Test: Multi-phase lifecycle member variable patterns
// Covers Init() → Work() → Release() lifecycle with exception handling

#include <cstddef>
#include <stdexcept>

// ========================================
// Pattern 1: Basic Two-Phase Initialization
// ========================================

class BasicLifecycle {
public:
  int* data_;
  bool initialized_;

  BasicLifecycle() : data_(nullptr), initialized_(false) {
  }

  void Init() {
    if (!initialized_) {
      data_ = new int(42);
      initialized_ = true;
    }
  }

  void Work() {
    // BUG: Should check if initialized before use
    *data_ = 100; // ERROR if Init() not called
  }

  void Release() {
    if (initialized_ && data_ != nullptr) {
      delete data_;
      data_ = nullptr;
      initialized_ = false;
    }
  }
};

void testBasicLifecycleUnsafe() {
  BasicLifecycle obj;
  // Forgot to call Init()
  obj.Work(); // ERROR: data_ is null
}

void testBasicLifecycleSafe() {
  BasicLifecycle obj;
  obj.Init();
  obj.Work(); // OK: initialized
  obj.Release();
}

void testBasicLifecycleAfterRelease() {
  BasicLifecycle obj;
  obj.Init();
  obj.Work();
  obj.Release();
  obj.Work(); // ERROR: used after Release, data_ is null
}

// ========================================
// Pattern 2: Exception-Safe Lifecycle
// ========================================

class ExceptionSafeLifecycle {
public:
  int* buffer_;
  size_t size_;

  ExceptionSafeLifecycle() : buffer_(nullptr), size_(0) {
  }

  void Allocate(size_t n) {
    if (buffer_ != nullptr) {
      throw std::runtime_error("Already allocated");
    }
    buffer_ = new int[n];
    size_ = n;
  }

  void Process() {
    if (buffer_ == nullptr) {
      throw std::runtime_error("Not allocated");
    }
    // Safe: checked first
    for (size_t i = 0; i < size_; ++i) {
      buffer_[i] = static_cast<int>(i);
    }
  }

  void ProcessUnsafe() {
    // BUG: No null check
    for (size_t i = 0; i < size_; ++i) {
      buffer_[i] = static_cast<int>(i); // ERROR if not allocated
    }
  }

  void Deallocate() {
    delete[] buffer_;
    buffer_ = nullptr;
    size_ = 0;
  }
};

void testExceptionSafeNormal() {
  ExceptionSafeLifecycle obj;
  obj.Allocate(100);
  obj.Process(); // OK
  obj.Deallocate();
}

void testExceptionSafeForgotAllocate() {
  ExceptionSafeLifecycle obj;
  obj.Process(); // Throws (safe) or ERROR in ProcessUnsafe
}

void testExceptionSafeUseAfterDealloc() {
  ExceptionSafeLifecycle obj;
  obj.Allocate(100);
  obj.Deallocate();
  obj.ProcessUnsafe(); // ERROR: used after deallocation
}

// ========================================
// Pattern 3: Complex Multi-Phase Resource
// ========================================

class MultiPhaseResource {
public:
  enum class State { Empty, Initialized, Processing, Released };

  int* data_;
  State state_;

  MultiPhaseResource() : data_(nullptr), state_(State::Empty) {
  }

  void Create() {
    if (state_ != State::Empty) {
      return; // Already created
    }
    data_ = new int(0);
    state_ = State::Initialized;
  }

  void Prepare() {
    if (state_ != State::Initialized) {
      return;
    }
    *data_ = 1;
    state_ = State::Processing;
  }

  void Execute() {
    // State check should ensure data_ is valid
    if (state_ != State::Processing) {
      return;
    }
    *data_ = 2; // OK: checked state
  }

  void ExecuteUnsafe() {
    // BUG: No state check
    *data_ = 2; // ERROR: may not be in Processing state
  }

  void Cleanup() {
    if (state_ == State::Released) {
      return;
    }
    delete data_;
    data_ = nullptr;
    state_ = State::Released;
  }

  void ExecuteAfterCleanup() {
    Cleanup();
    ExecuteUnsafe(); // ERROR: after cleanup, data_ is null
  }
};

void testMultiPhaseCorrect() {
  MultiPhaseResource res;
  res.Create();
  res.Prepare();
  res.Execute(); // OK
  res.Cleanup();
}

void testMultiPhaseSkipPrepare() {
  MultiPhaseResource res;
  res.Create();
  // Skipped Prepare(), still in Initialized state
  res.ExecuteUnsafe(); // BUG: wrong state, but data_ is not null
}

void testMultiPhaseDoubleCleanup() {
  MultiPhaseResource res;
  res.Create();
  res.Cleanup();
  res.Cleanup(); // Double cleanup is safe but data_ already null
}

// ========================================
// Pattern 4: Connection Pool Lifecycle
// ========================================

class Connection {
public:
  bool connected_;
  int* socket_data_;

  Connection() : connected_(false), socket_data_(nullptr) {
  }

  void Connect() {
    if (connected_)
      return;
    socket_data_ = new int(1); // Simulate socket
    connected_ = true;
  }

  void Send(const char* data) {
    // BUG: No connection check
    *socket_data_ = 2; // ERROR if not connected
  }

  void SendSafe(const char* data) {
    if (!connected_ || socket_data_ == nullptr) {
      return; // Not connected
    }
    *socket_data_ = 2; // OK: checked
  }

  void Disconnect() {
    if (!connected_)
      return;
    delete socket_data_;
    socket_data_ = nullptr;
    connected_ = false;
  }
};

void testConnectionNormal() {
  Connection conn;
  conn.Connect();
  conn.SendSafe("hello"); // OK
  conn.Disconnect();
}

void testConnectionNoConnect() {
  Connection conn;
  conn.Send("hello"); // ERROR: never connected
}

void testConnectionAfterDisconnect() {
  Connection conn;
  conn.Connect();
  conn.Disconnect();
  conn.Send("hello"); // ERROR: disconnected
}

// ========================================
// Pattern 5: Lazy Initialization with Cache
// ========================================

class LazyCache {
public:
  int* cached_value_;
  bool computed_;

  LazyCache() : cached_value_(nullptr), computed_(false) {
  }

  void EnsureComputed() {
    if (!computed_) {
      cached_value_ = new int(42);
      computed_ = true;
    }
  }

  int GetValue() {
    // BUG: May not be computed yet
    return *cached_value_; // ERROR if EnsureComputed not called
  }

  int GetValueSafe() {
    EnsureComputed();
    return *cached_value_; // OK: ensured
  }

  void Invalidate() {
    delete cached_value_;
    cached_value_ = nullptr;
    computed_ = false;
  }
};

void testLazyCacheDirect() {
  LazyCache cache;
  int x = cache.GetValue(); // ERROR: not computed
}

void testLazyCacheSafe() {
  LazyCache cache;
  int x = cache.GetValueSafe(); // OK
}

void testLazyCacheAfterInvalidate() {
  LazyCache cache;
  cache.EnsureComputed();
  cache.Invalidate();
  int x = cache.GetValue(); // ERROR: invalidated
}

// ========================================
// Pattern 6: Resettable Resource
// ========================================

class ResettableResource {
public:
  int* resource_;

  ResettableResource() : resource_(nullptr) {
  }

  void Acquire() {
    if (resource_ == nullptr) {
      resource_ = new int(100);
    }
  }

  void Reset() {
    delete resource_;
    resource_ = nullptr;
  }

  void Use() {
    *resource_ = 200; // ERROR if Acquire not called or after Reset
  }

  void UseSafe() {
    if (resource_ != nullptr) {
      *resource_ = 200; // OK: checked
    }
  }
};

void testResettableNormal() {
  ResettableResource res;
  res.Acquire();
  res.Use(); // OK
  res.Reset();
}

void testResettableMultipleCycles() {
  ResettableResource res;
  res.Acquire();
  res.Use(); // OK
  res.Reset();
  res.Acquire();
  res.Use(); // OK again
  res.Reset();
  res.Use(); // ERROR: after final reset
}

// ========================================
// Pattern 7: Exception During Initialization
// ========================================

class ExceptionInInit {
public:
  int* part1_;
  int* part2_;

  ExceptionInInit() : part1_(nullptr), part2_(nullptr) {
  }

  void Init() {
    part1_ = new int(1);
    if (someCondition()) {
      throw std::runtime_error("Init failed");
    }
    part2_ = new int(2);
  }

  void Work() {
    *part1_ = 10; // OK if Init succeeded
    *part2_ = 20; // May be null if exception thrown
  }

  void Cleanup() {
    delete part1_;
    delete part2_;
    part1_ = nullptr;
    part2_ = nullptr;
  }

  bool someCondition() {
    return false;
  }
};

void testExceptionInInitNormal() {
  ExceptionInInit obj;
  try {
    obj.Init();
    obj.Work(); // OK if no exception
  } catch (...) {
    // Handle error
  }
  obj.Cleanup();
}

// ========================================
// Pattern 8: Ownership Transfer
// ========================================

class OwnershipTransfer {
public:
  int* owned_;

  OwnershipTransfer() : owned_(nullptr) {
  }

  void TakeOwnership(int* ptr) {
    if (owned_ != nullptr) {
      delete owned_;
    }
    owned_ = ptr;
  }

  int* ReleaseOwnership() {
    int* tmp = owned_;
    owned_ = nullptr;
    return tmp;
  }

  void Use() {
    *owned_ = 42; // ERROR if ownership released or not taken
  }
};

void testOwnershipNormal() {
  OwnershipTransfer owner;
  int* data = new int(10);
  owner.TakeOwnership(data);
  owner.Use(); // OK
  int* released = owner.ReleaseOwnership();
  delete released;
}

void testOwnershipUseAfterRelease() {
  OwnershipTransfer owner;
  owner.TakeOwnership(new int(10));
  owner.ReleaseOwnership();
  owner.Use(); // ERROR: ownership released
}

void testOwnershipNeverTaken() {
  OwnershipTransfer owner;
  owner.Use(); // ERROR: never took ownership
}

// ========================================
// Pattern 9: Conditional Lifecycle
// ========================================

class ConditionalLifecycle {
public:
  int* data_;
  bool should_init_;

  ConditionalLifecycle(bool should_init) : data_(nullptr), should_init_(should_init) {
  }

  void MaybeInit() {
    if (should_init_) {
      data_ = new int(42);
    }
  }

  void Work() {
    *data_ = 100; // ERROR if should_init_ was false
  }

  void WorkSafe() {
    if (data_ != nullptr) {
      *data_ = 100; // OK
    }
  }
};

void testConditionalWithInit() {
  ConditionalLifecycle obj(true);
  obj.MaybeInit();
  obj.Work(); // OK
}

void testConditionalWithoutInit() {
  ConditionalLifecycle obj(false);
  obj.MaybeInit(); // Won't init
  obj.Work();      // ERROR: data_ is null
}

// ========================================
// Pattern 10: Reentrant Resource
// ========================================

class ReentrantResource {
public:
  int* shared_data_;
  int ref_count_;

  ReentrantResource() : shared_data_(nullptr), ref_count_(0) {
  }

  void Open() {
    if (ref_count_ == 0) {
      shared_data_ = new int(0);
    }
    ++ref_count_;
  }

  void Close() {
    --ref_count_;
    if (ref_count_ == 0) {
      delete shared_data_;
      shared_data_ = nullptr;
    }
  }

  void Access() {
    *shared_data_ = 1; // ERROR if all Close() called
  }

  void AccessSafe() {
    if (ref_count_ > 0 && shared_data_ != nullptr) {
      *shared_data_ = 1; // OK
    }
  }
};

void testReentrantNormal() {
  ReentrantResource res;
  res.Open();
  res.Access(); // OK
  res.Close();
}

void testReentrantRefCountZero() {
  ReentrantResource res;
  res.Open();
  res.Close();
  res.Access(); // ERROR: refcount is 0
}

void testReentrantMultipleOpenClose() {
  ReentrantResource res;
  res.Open();
  res.Open();   // refcount = 2
  res.Close();  // refcount = 1, data still valid
  res.Access(); // OK: refcount > 0
  res.Close();  // refcount = 0, data deleted
  res.Access(); // ERROR
}
