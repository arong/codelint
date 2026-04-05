#include <iostream>

// Curiously Recurring Template Pattern Singleton
template<typename T>
class SingletonBase {
public:
    static T& instance() {
        static T t; // CRTP singleton implementation
        return static_cast<T&>(t);
    }

protected:
    SingletonBase() = default;
    ~SingletonBase() = default;
};

class ConcreteImpl : public SingletonBase<ConcreteImpl> {
public:
    void doWork() {
        std::cout << "CRTP Singleton working..." << std::endl;
    }

private:
    friend class SingletonBase<ConcreteImpl>;
    ConcreteImpl() = default;
};

int main() {
    ConcreteImpl& impl = ConcreteImpl::instance();
    impl.doWork();
    return 0;
}