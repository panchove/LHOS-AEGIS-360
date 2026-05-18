
# Consider dependencies only in project.
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

# The set of languages for which implicit dependencies are needed:
set(CMAKE_DEPENDS_LANGUAGES
  "ASM"
  )
# The set of files for implicit dependencies of each language:
set(CMAKE_DEPENDS_CHECK_ASM
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/port/xtensa/core_dump_stack_switch.S" "/home/panchove/Projects/LHOS-AEGIS-360/build/esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/port/xtensa/core_dump_stack_switch.S.obj"
  )
set(CMAKE_ASM_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_ASM
  "ESP_PLATFORM"
  "IDF_VER=\"v5.5.3\""
  "MBEDTLS_CONFIG_FILE=\"mbedtls/esp_config.h\""
  "SOC_MMU_PAGE_SIZE=CONFIG_MMU_PAGE_SIZE"
  "SOC_XTAL_FREQ_MHZ=CONFIG_XTAL_FREQ"
  "_GLIBCXX_HAVE_POSIX_SEMAPHORE"
  "_GLIBCXX_USE_POSIX_SEMAPHORE"
  "_GNU_SOURCE"
  "_POSIX_READER_WRITER_LOCKS"
  )

# The include file search paths:
set(CMAKE_ASM_TARGET_INCLUDE_PATH
  "config"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/include/port/xtensa"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/include_core_dump"
  "/home/panchove/esp/esp-idf-v5.5.3/components/newlib/platform_include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/freertos/config/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/freertos/config/include/freertos"
  "/home/panchove/esp/esp-idf-v5.5.3/components/freertos/config/xtensa/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/freertos/FreeRTOS-Kernel/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/freertos/FreeRTOS-Kernel/portable/xtensa/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos"
  "/home/panchove/esp/esp-idf-v5.5.3/components/freertos/esp_additions/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/include/soc"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/include/soc/esp32"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/dma/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/ldo/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/debug_probe/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/mspi_timing_tuning/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/mspi_timing_tuning/tuning_scheme_impl/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/power_supply/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/port/esp32/."
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_hw_support/port/esp32/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/heap/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/heap/tlsf"
  "/home/panchove/esp/esp-idf-v5.5.3/components/log/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/soc/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/soc/esp32"
  "/home/panchove/esp/esp-idf-v5.5.3/components/soc/esp32/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/soc/esp32/register"
  "/home/panchove/esp/esp-idf-v5.5.3/components/hal/platform_port/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/hal/esp32/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/hal/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_rom/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_rom/esp32/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_rom/esp32/include/esp32"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_rom/esp32"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_common/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_system/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_system/port/soc"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_system/port/include/private"
  "/home/panchove/esp/esp-idf-v5.5.3/components/xtensa/esp32/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/xtensa/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/xtensa/deprecated_include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/include/apps"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/include/apps/sntp"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/lwip/src/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/port/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/port/freertos/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/port/esp32xx/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/port/esp32xx/include/arch"
  "/home/panchove/esp/esp-idf-v5.5.3/components/lwip/port/esp32xx/include/sys"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_partition/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/spi_flash/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/bootloader_support/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/bootloader_support/bootloader_flash/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/mbedtls/port/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/mbedtls/mbedtls/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/mbedtls/mbedtls/library"
  "/home/panchove/esp/esp-idf-v5.5.3/components/mbedtls/esp_crt_bundle/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/mbedtls/mbedtls/3rdparty/everest/include"
  "/home/panchove/esp/esp-idf-v5.5.3/components/mbedtls/mbedtls/3rdparty/p256-m"
  "/home/panchove/esp/esp-idf-v5.5.3/components/mbedtls/mbedtls/3rdparty/p256-m/p256-m"
  "/home/panchove/esp/esp-idf-v5.5.3/components/esp_driver_gpio/include"
  )

# The set of dependency files which are needed:
set(CMAKE_DEPENDS_DEPENDENCY_FILES
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_binary.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_binary.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_binary.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_common.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_common.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_common.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_crc.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_crc.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_crc.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_elf.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_elf.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_elf.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_flash.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_flash.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_flash.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_init.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_init.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_init.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_sha.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_sha.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_sha.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/core_dump_uart.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_uart.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/core_dump_uart.c.obj.d"
  "/home/panchove/esp/esp-idf-v5.5.3/components/espcoredump/src/port/xtensa/core_dump_port.c" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/port/xtensa/core_dump_port.c.obj" "gcc" "esp-idf/espcoredump/CMakeFiles/__idf_espcoredump.dir/src/port/xtensa/core_dump_port.c.obj.d"
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_LINKED_INFO_FILES
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_FORWARD_LINKED_INFO_FILES
  )

# Fortran module output directory.
set(CMAKE_Fortran_TARGET_MODULE_DIR "")
