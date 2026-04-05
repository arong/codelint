// Test file: template_globals.cpp
// Scenario: Template variables
// Expected: template variable instances detected

template<typename T>
T template_var = T{};

template<> int template_var<int> = 42;

double d = template_var<double>;

int main() {
    return 0;
}
