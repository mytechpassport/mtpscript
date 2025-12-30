#include "src/test/acceptance_tests.c"
int main() {
    bool result = test_openapi_deterministic_ordering();
    printf("Test result: %s
", result ? "PASS" : "FAIL");
    return 0;
}
