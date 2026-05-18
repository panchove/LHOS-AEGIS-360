# CMakeLists.txt para test bundles ESP-IDF Unity
idf_component_register(SRCS "test_pool.c" "test_parser.c" "test_crc.c" "test_message_bus.c" "test_blackbox.c"
    INCLUDE_DIRS "../src/core")
