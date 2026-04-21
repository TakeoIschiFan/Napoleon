#include "../napoleon.h"

/* Math tests */
void test_addition(void) { nap_assert(2 + 2 == 4); }

void test_subtraction(void) { nap_assert(5 - 3 == 2); }

void test_multiplication(void) { nap_assert(4 * 3 == 12); }

void test_division(void) { nap_assert(10 / 2 == 5); }

void test_floating_point(void) { nap_assert_num(1.0 / 5.0, 0.333333, 0.0001); }

void test_num(void) { nap_assert_num(5, 10); }

void test_num_equal(void) { nap_assert_num(5, 5); }

/* String tests */
void test_string_equal(void) {
  const char *a = "hello";
  const char *b = "hello";
  nap_assert_str(a, b);
}

void test_string_unequal(void) {
  const char *a = "hello";
  const char *b = "world";
  nap_assert(a != b);
}

void test_string_mismatch(void) { nap_assert_str("foo", "bar"); }

/* Memory tests */
void test_memcmp_equal(void) {
  int a[4] = {1, 2, 3, 4};
  int b[4] = {1, 2, 3, 4};
  nap_assert_mem(a, b, sizeof(a));
}

void test_memcmp_unequal(void) {
  int a[4] = {1, 2, 3, 4};
  int b[4] = {1, 2, 3, 5};
  nap_assert(memcmp(a, b, sizeof(a)) != 0);
}

/* Self-test: known failing assertions (should be skipped or pass) */
void test_true_is_true(void) { nap_assert(true == true); }

void test_false_is_false(void) { nap_assert(false == false); }

/* WIP feature test - skipped for demonstration */
void test_wip_feature(void) { nap_assert(false); }

/* Test that waits for one second */
void test_wait_one_second(void) { sleep(1); }

/* Test that waits for 30 seconds (should timeout) */
void test_wait_thirty_seconds(void) { sleep(30); }

/* Test that segfaults (should fail) */
void test_segfault(void) { *(int *)0 = 42; }

/* Register all tests */
void nap_register_tests(void) {
  nap_add(test_addition);
  nap_add(test_subtraction, .suite = "basic_math");
  nap_add(test_multiplication, .suite = "basic_math");
  nap_add(test_division, .suite = "basic_math");
  nap_add(test_floating_point, .suite = "basic_math");
  nap_add(test_num, .suite = "basic_math");
  nap_add(test_num_equal, .suite = "basic_math");
  nap_add(test_string_equal, .suite = "strings");
  nap_add(test_string_unequal, .suite = "strings");
  nap_add(test_string_mismatch, .suite = "strings");
  nap_add(test_memcmp_equal, .suite = "memory");
  nap_add(test_memcmp_unequal, .suite = "memory");
  nap_add(test_true_is_true);
  nap_add(test_false_is_false);
  nap_add(test_wip_feature, .skip_reason = "Work in progress");
  nap_add(test_wait_one_second);
  nap_add(test_wait_thirty_seconds);
  nap_add(test_segfault);
}
