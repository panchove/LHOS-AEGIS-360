# CMakeLists.txt para test bundles ESP-IDF Unity
idf_component_register(SRCS "test_pool.cpp" "test_crc16.cpp" "test_parser.cpp" "test_runner.cpp"
    INCLUDE_DIRS "."
    REQUIRES pool crc parser)
