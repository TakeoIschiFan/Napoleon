#include "../napoleon.h"
#include <stdio.h>

void test_basic(void) { nap_assert(true); }

void test_strings(void) { nap_assert_str("my string should equal", "my string should equal"); }

void test_captured_output(void) {
    fprintf(stdout, "captured stdout\n");
    fprintf(stderr, "captured stderr\n");
    nap_assert(true);
}

void test_numerics(void) { nap_assert_num(24, 24); }

void test_numerics_with_tolerance(void) { nap_assert_num(1.0f / 3.0f, 0.333f, .tolerance = 0.01f); }

void test_memory(void) {
    int a[4] = {1, 2, 3, 4};
    int b[4] = {1, 2, 3, 4};
    nap_assert_mem(a, b, sizeof(a));
}

void test_secondary(void) { nap_assert(true); }

void test_skipped(void) { nap_assert(true); }

void test_timeout(void) {
    sleep(2);
    nap_assert(true);
}

void test_expected_failure_passes(void) { nap_assert(false); }

void test_sleep_5_seconds(void) {
    sleep(5);
    nap_assert(true);
}

void register_tests(void) {
    nap_add(test_basic);
    nap_add(test_strings, .suite = "asserts");
    nap_add(test_numerics, .suite = "asserts");
    nap_add(test_numerics_with_tolerance, .suite = "asserts");
    nap_add(test_memory, .suite = "asserts");
    nap_add(test_secondary, .suite = "secondary suite");
    nap_add(test_skipped, .skip_reason = "not implemented yet");
    nap_add(test_timeout);
    nap_add(test_captured_output);
    nap_add(test_expected_failure_passes, .should_fail = true);
    nap_add(test_sleep_5_seconds, .should_fail = true, .timeout = 1);
}
