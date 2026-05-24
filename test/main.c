#include <unity.h>
#include "test_mr.h"
#include "test_log.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

int main(void) {
    UNITY_BEGIN();
    test_mr();
    test_log();
    return UNITY_END();
}
