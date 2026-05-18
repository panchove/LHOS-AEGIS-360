set(TEST_NAME "unit_tests")
idf_component_register(SRCS "test_crc.c" "test_pool.c" INCLUDE_DIRS "../components/pool/include" "../components/crc/include" "../components/parser/include" REQUIRES unity)
