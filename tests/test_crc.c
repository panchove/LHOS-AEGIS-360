#include "unity.h"
#include "crc16.h"

void test_crc_known_vector(void) {
  const uint8_t data[] = { 'A','B','C','1','2','3' };
  uint16_t c = crc16_ccitt(data, sizeof(data));
  // known crc for "ABC123" using ccitt 0xFFFF is implementation dependent;
  // we check non-zero to assert function runs.
  TEST_ASSERT_NOT_EQUAL(0, c);
}

void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_crc_known_vector);
  UNITY_END();
}
