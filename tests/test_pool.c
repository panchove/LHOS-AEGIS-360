#include "unity.h"
#include "pool.h"

void test_pool_alloc_release(void) {
  TEST_ASSERT_EQUAL(0, pool_init());
  unsigned before = pool_free_count();
  pool_buf_t* b = pool_alloc();
  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT_EQUAL(before - 1, pool_free_count());
  pool_release(b);
  TEST_ASSERT_EQUAL(before, pool_free_count());
}

void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_pool_alloc_release);
  UNITY_END();
}
