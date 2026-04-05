#include <iostream>

// Thread-Local Singleton
class ThreadLocalService {
public:
    static ThreadLocalService& instance() {
        static thread_local ThreadLocalService svc; // thread_local singleton
        return svc;
    }

    void work() {
        std::cout << "Working with thread-local singleton..." << std::endl;
    }

private:
    ThreadLocalService() = default;
    ThreadLocalService(const ThreadLocalService&) = delete;
    ThreadLocalService& operator=(const ThreadLocalService&) = delete;
};

int main() {
    ThreadLocalService& tls = ThreadLocalService::instance();
    tls.work();
    return 0;
}