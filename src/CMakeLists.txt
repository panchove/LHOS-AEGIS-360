# CMakeLists.txt para src/core como componente ESP-IDF
idf_component_register(SRCS "core/pool.c" "core/parser.c" "core/crc16.c" "core/message_bus.c" "core/blackbox.c" "core/gzip.c"
                      INCLUDE_DIRS "core")
