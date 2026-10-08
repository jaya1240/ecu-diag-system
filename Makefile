# Makefile for building against a locally installed STM32Cube HAL package.
#
# Usage:
#   make TARGET=stm32f103xb HAL_DIR=/path/to/STM32CubeF1
#   make flash          # requires st-flash (stlink-tools) or OpenOCD, see below
#   make clean
#
# This Makefile intentionally does NOT vendor ST's HAL/CMSIS sources into
# this repo (licensing/size) — point HAL_DIR at your local STM32CubeF1 (or
# STM32CubeF4, etc.) checkout, or just use STM32CubeIDE (see README).

TARGET      ?= stm32f103xb
HAL_DIR     ?= /opt/STM32CubeF1
BUILD_DIR   := build
ELF         := $(BUILD_DIR)/ecu-diag-system.elf
BIN         := $(BUILD_DIR)/ecu-diag-system.bin

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CPU_FLAGS := -mcpu=cortex-m3 -mthumb

SRC := \
	Core/Src/main.c \
	Core/Src/can_driver.c \
	Core/Src/uart_driver.c \
	Core/Src/diagnostics.c \
	Core/Src/ring_buffer.c \
	Core/Src/stm32f1xx_it.c \
	Core/Src/system_stm32f1xx.c

ASM_SRC := Startup/startup_stm32f103xb.s

INC := \
	-ICore/Inc \
	-I$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Inc \
	-I$(HAL_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Include \
	-I$(HAL_DIR)/Drivers/CMSIS/Include

DEFS := -DSTM32F103xB -DUSE_HAL_DRIVER

# Only the HAL driver source files this project actually uses — link errors
# for a missing symbol usually mean another *_HAL_*.c needs adding here.
HAL_SRC := \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_can.c \
	$(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c

CFLAGS  := $(CPU_FLAGS) $(DEFS) $(INC) -Wall -Wextra -Os -ffunction-sections -fdata-sections -g
ASFLAGS := $(CPU_FLAGS) -g
LDFLAGS := $(CPU_FLAGS) -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs \
           -TSTM32F103C8Tx_FLASH.ld

OBJ := $(SRC:%.c=$(BUILD_DIR)/%.o) $(HAL_SRC:%.c=$(BUILD_DIR)/hal/%.o) $(ASM_SRC:%.s=$(BUILD_DIR)/%.o)

.PHONY: all clean flash

all: $(BIN)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hal/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(ELF): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

flash: $(BIN)
	st-flash write $(BIN) 0x8000000

clean:
	rm -rf $(BUILD_DIR)
