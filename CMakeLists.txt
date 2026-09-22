cmake_minimum_required(VERSION 3.20)
project(stm32f4_accelerometer_example C ASM)

# ── Accelerometer source selection ──────────────────────────────────────────
# "real"      -> src/accel_real.c       (SPI1 driver for the onboard
#                LIS3DSH/LIS302DL, real hardware only)
# "synthetic" -> src/accel_synthetic.c  (CSV-derived playback, for Renode --
#                Renode has no model for either accelerometer chip; see
#                renode/README or ../README.md's "Renode" section)
set(ACCEL_SOURCE "real" CACHE STRING "Accelerometer source: real or synthetic")
set_property(CACHE ACCEL_SOURCE PROPERTY STRINGS real synthetic)

if(ACCEL_SOURCE STREQUAL "real")
    set(ACCEL_SRC src/accel_real.c)
elseif(ACCEL_SOURCE STREQUAL "synthetic")
    set(ACCEL_SRC src/accel_synthetic.c)
else()
    message(FATAL_ERROR "ACCEL_SOURCE must be 'real' or 'synthetic', got: ${ACCEL_SOURCE}")
endif()

add_executable(firmware.elf
    startup.c
    src/main.c
    src/hal.c
    ${ACCEL_SRC}
)

target_include_directories(firmware.elf PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_options(firmware.elf PRIVATE
    -std=c11
    -Wall -Wextra
    -Wno-unused-parameter
    -Os -g3
    -ffunction-sections
    -fdata-sections
    -ffreestanding
    -fno-builtin
    -DSTM32F407xx
)

target_link_options(firmware.elf PRIVATE
    -T${CMAKE_SOURCE_DIR}/STM32F407VG.ld
    -nostartfiles
    -nostdlib
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map,--cref
    -Wl,--print-memory-usage
)

# Generate .bin for flashing with OpenOCD
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary firmware.elf firmware.bin
    COMMENT "Generating firmware.bin"
)

# Print section sizes
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND arm-none-eabi-size firmware.elf
    COMMENT "Firmware size:"
)

# ── Helper targets ─────────────────────────────────────────────────────────

add_custom_target(flash
    COMMAND openocd
        -f interface/stlink.cfg
        -f target/stm32f4x.cfg
        -c "transport select hla_swd"
        -c "program ${CMAKE_BINARY_DIR}/firmware.bin 0x08000000 verify reset exit"
    DEPENDS firmware.elf
    COMMENT "Flashing firmware via OpenOCD (chip runs after)"
    USES_TERMINAL
)

add_custom_target(debug
    COMMAND openocd
        -f interface/stlink.cfg
        -f target/stm32f4x.cfg
        -c "transport select hla_swd"
    DEPENDS firmware.elf
    COMMENT "Starting OpenOCD GDB server on :3333 (Ctrl-C to stop)"
    USES_TERMINAL
)

add_custom_target(disasm
    COMMAND arm-none-eabi-objdump -d -S ${CMAKE_BINARY_DIR}/firmware.elf
    DEPENDS firmware.elf
    COMMENT "Disassembly:"
    USES_TERMINAL
)
