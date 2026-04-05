#include <iostream>

// Const-Correct Singleton
class Service {
public:
    static const Service& instance() {
        static Service svc; // const reference to singleton
        return svc;
    }

    void serve() const {
        std::cout << "Providing service..." << std::endl;
    }

private:
    Service() = default;
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
};

int main() {
    const Service& svc = Service::instance();
    svc.serve();
    return 0;
}