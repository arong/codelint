// Test: Send/Work workflow patterns with null checks
// Simulates real-world APIs like graphics, network, or database operations

#include <cstddef>

// ========================================
// Pattern: Graphics Buffer Lifecycle
// ========================================

class GraphicsBuffer {
public:
  unsigned char* pixels_;
  int width_;
  int height_;
  bool locked_;

  GraphicsBuffer() : pixels_(nullptr), width_(0), height_(0), locked_(false) {
  }

  bool Create(int w, int h) {
    if (pixels_ != nullptr) {
      return false;
    }
    pixels_ = new unsigned char[w * h * 4];
    width_ = w;
    height_ = h;
    return true;
  }

  unsigned char* Lock() {
    if (pixels_ == nullptr || locked_) {
      return nullptr;
    }
    locked_ = true;
    return pixels_;
  }

  void Unlock() {
    locked_ = false;
  }

  void ClearUnsafe(int color) {
    for (int i = 0; i < width_ * height_ * 4; ++i) {
      pixels_[i] = static_cast<unsigned char>(color);
    }
  }

  void ClearSafe(int color) {
    if (pixels_ == nullptr) {
      return;
    }
    for (int i = 0; i < width_ * height_ * 4; ++i) {
      pixels_[i] = static_cast<unsigned char>(color);
    }
  }

  void Destroy() {
    delete[] pixels_;
    pixels_ = nullptr;
    width_ = 0;
    height_ = 0;
    locked_ = false;
  }
};

void testGraphicsNormal() {
  GraphicsBuffer buf;
  buf.Create(100, 100);
  unsigned char* pixels = buf.Lock();
  if (pixels != nullptr) {
    pixels[0] = 255;
  }
  buf.Unlock();
  buf.ClearSafe(0);
  buf.Destroy();
}

void testGraphicsUseBeforeCreate() {
  GraphicsBuffer buf;
  buf.ClearUnsafe(0);
}

void testGraphicsUseAfterDestroy() {
  GraphicsBuffer buf;
  buf.Create(100, 100);
  buf.Destroy();
  buf.ClearUnsafe(0);
}

void testGraphicsLockAfterDestroy() {
  GraphicsBuffer buf;
  buf.Create(100, 100);
  buf.Destroy();
  unsigned char* pixels = buf.Lock();
  pixels[0] = 255;
}

// ========================================
// Pattern: Network Send/Receive
// ========================================

class NetworkSocket {
public:
  int* socket_handle_;
  bool connected_;

  NetworkSocket() : socket_handle_(nullptr), connected_(false) {
  }

  bool Connect(const char* address, int port) {
    if (connected_) {
      return true;
    }
    socket_handle_ = new int(1);
    connected_ = true;
    return true;
  }

  void SendUnsafe(const char* data, int len) {
    *socket_handle_ = 2;
  }

  void SendSafe(const char* data, int len) {
    if (!connected_ || socket_handle_ == nullptr) {
      return;
    }
    *socket_handle_ = 2;
  }

  int ReceiveUnsafe(char* buffer, int maxLen) {
    *socket_handle_ = 3;
    return 0;
  }

  void Disconnect() {
    if (socket_handle_ != nullptr) {
      delete socket_handle_;
      socket_handle_ = nullptr;
    }
    connected_ = false;
  }
};

void testNetworkNormal() {
  NetworkSocket sock;
  sock.Connect("localhost", 8080);
  sock.SendSafe("hello", 5);
  sock.Disconnect();
}

void testNetworkSendBeforeConnect() {
  NetworkSocket sock;
  sock.SendUnsafe("hello", 5);
}

void testNetworkSendAfterDisconnect() {
  NetworkSocket sock;
  sock.Connect("localhost", 8080);
  sock.Disconnect();
  sock.SendUnsafe("hello", 5);
}

// ========================================
// Pattern: Database Connection
// ========================================

class DatabaseConnection {
public:
  int* db_handle_;
  bool in_transaction_;

  DatabaseConnection() : db_handle_(nullptr), in_transaction_(false) {
  }

  bool Open(const char* connection_string) {
    if (db_handle_ != nullptr) {
      return false;
    }
    db_handle_ = new int(100);
    return true;
  }

  void BeginTransaction() {
    if (db_handle_ == nullptr) {
      return;
    }
    in_transaction_ = true;
    *db_handle_ = 200;
  }

  void ExecuteUnsafe(const char* sql) {
    *db_handle_ = 300;
    in_transaction_ = true;
  }

  void Commit() {
    if (db_handle_ != nullptr && in_transaction_) {
      *db_handle_ = 400;
      in_transaction_ = false;
    }
  }

  void Close() {
    delete db_handle_;
    db_handle_ = nullptr;
    in_transaction_ = false;
  }
};

void testDatabaseNormal() {
  DatabaseConnection conn;
  conn.Open("server=localhost");
  conn.BeginTransaction();
  conn.Commit();
  conn.Close();
}

void testDatabaseExecuteBeforeOpen() {
  DatabaseConnection conn;
  conn.ExecuteUnsafe("SELECT * FROM users");
}

void testDatabaseCommitAfterClose() {
  DatabaseConnection conn;
  conn.Open("server=localhost");
  conn.BeginTransaction();
  conn.Close();
  conn.Commit();
}

// ========================================
// Pattern: Audio Buffer
// ========================================

class AudioBuffer {
public:
  float* samples_;
  int sample_count_;
  bool is_processing_;

  AudioBuffer() : samples_(nullptr), sample_count_(0), is_processing_(false) {
  }

  bool Allocate(int num_samples) {
    if (samples_ != nullptr) {
      return false;
    }
    samples_ = new float[num_samples];
    sample_count_ = num_samples;
    return true;
  }

  void ProcessEffects() {
    if (samples_ == nullptr) {
      return;
    }
    is_processing_ = true;
    for (int i = 0; i < sample_count_; ++i) {
      samples_[i] *= 0.5f;
    }
    is_processing_ = false;
  }

  void MixUnsafe(const AudioBuffer& other) {
    for (int i = 0; i < sample_count_; ++i) {
      samples_[i] += other.samples_[i];
    }
  }

  void Deallocate() {
    delete[] samples_;
    samples_ = nullptr;
    sample_count_ = 0;
    is_processing_ = false;
  }
};

void testAudioNormal() {
  AudioBuffer buf;
  buf.Allocate(1024);
  buf.ProcessEffects();
  buf.Deallocate();
}

void testAudioMixBeforeAllocate() {
  AudioBuffer buf1;
  AudioBuffer buf2;
  buf1.Allocate(1024);
  buf1.MixUnsafe(buf2);
}
