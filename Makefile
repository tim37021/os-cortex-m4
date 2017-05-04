PROJECT = project

EXECUTABLE = $(PROJECT).elf
BIN_IMAGE = $(PROJECT).bin
HEX_IMAGE = $(PROJECT).hex

# set the path to STM32F429I-Discovery firmware package
STDP ?= ../STM32F429I-Discovery_FW_V1.0.1
USB_LIB ?= ./usb_lib

# Toolchain configurations
CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

# Cortex-M4 implements the ARMv7E-M architecture
CPU = cortex-m4
CFLAGS = -mcpu=$(CPU) -march=armv7e-m -mtune=cortex-m4
CFLAGS += -mlittle-endian -mthumb
# FPU
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard

LDFLAGS =
define get_library_path
    $(shell dirname $(shell $(CC) $(CFLAGS) -print-file-name=$(1)))
endef
LDFLAGS += -L $(call get_library_path,libc.a)
LDFLAGS += -L $(call get_library_path,libgcc.a)

# Basic configurations
CFLAGS += -g -std=c99
CFLAGS += -Wall

# Optimizations
CFLAGS += -ffast-math
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wl,--gc-sections
CFLAGS += -fno-common
CFLAGS += --param max-inline-insns-single=1000

# FPU
CFLAGS += -DARM_MATH_CM4 -D__FPU_PRESENT

# specify STM32F429
CFLAGS += -DSTM32F429_439xx

# to run from FLASH
CFLAGS += -DVECT_TAB_FLASH
LDFLAGS += -T stm32f429zi_flash.ld

# project starts here
CFLAGS += -I.
OBJS = \
    main.o \
	ltdc.o \
	utility.o \
	keybd_stm32.o \
	context_switch.o \
	keybd.o \
	priority_queue.o \
    stm32f4xx_it.o \
	malloc.o \
    system_stm32f4xx.o

# L3GD20 default callback
# CFLAGS += -DUSE_DEFAULT_TIMEOUT_CALLBACK

# STARTUP FILE
OBJS += startup_stm32f429_439xx.o

# CMSIS
CFLAGS += -I$(STDP)/Libraries/CMSIS/Device/ST/STM32F4xx/Include
CFLAGS += -I$(STDP)/Libraries/CMSIS/Include
LIBS += $(STDP)/Libraries/CMSIS/Lib/GCC/libarm_cortexM4lf_math.a

# STemWinLibrary522_4x9i
CFLAGS += -I$(STDP)/Libraries/STemWinLibrary522_4x9i/inc
LIBS += \
    $(STDP)/Libraries/STemWinLibrary522_4x9i/Lib/STemWin522_4x9i_CM4_OS_GCC.a

# STM32F4xx_StdPeriph_Driver
CFLAGS += -DUSE_STDPERIPH_DRIVER
CFLAGS += -I$(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/inc
CFLAGS += -I$(USB_LIB) -I$(USB_LIB)/usb_hid_device
CFLAGS += -D"assert_param(expr)=((void)0)"

OBJS_USB += \
	$(USB_LIB)/tm_stm32f4_usb_hid_device.o \
	$(USB_LIB)/usb_hid_device/usb_bsp.o \
	$(USB_LIB)/usb_hid_device/usb_core.o \
	$(USB_LIB)/usb_hid_device/usb_dcd.o \
	$(USB_LIB)/usb_hid_device/usb_dcd_int.o \
	$(USB_LIB)/usb_hid_device/usbd_core.o \
	$(USB_LIB)/usb_hid_device/usbd_desc.o \
	$(USB_LIB)/usb_hid_device/usbd_ioreq.o \
	$(USB_LIB)/usb_hid_device/usbd_usr.o \
	$(USB_LIB)/usb_hid_device/usbd_req.o \
	$(USB_LIB)/usb_hid_device/usbd_hid_core.o \



OBJS += \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/misc.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma2d.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_exti.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_flash.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_fmc.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_i2c.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_ltdc.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_syscfg.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_spi.o \
    $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.o \
	$(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_usart.o \
	$(OBJS_USB)

# STM32F429I-Discovery Utilities
CFLAGS += -I$(STDP)/Utilities/STM32F429I-Discovery
OBJS += $(STDP)/Utilities/STM32F429I-Discovery/stm32f429i_discovery.o
#OBJS += $(STDP)/Utilities/STM32F429I-Discovery/stm32f429i_discovery_l3gd20.o
OBJS += $(STDP)/Utilities/STM32F429I-Discovery/stm32f429i_discovery_lcd.o
OBJS += $(STDP)/Utilities/STM32F429I-Discovery/stm32f429i_discovery_sdram.o

all: $(BIN_IMAGE)

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@
	$(OBJCOPY) -O ihex $^ $(HEX_IMAGE)
	$(OBJDUMP) -h -S -D $(EXECUTABLE) > $(PROJECT).lst
	$(SIZE) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJS)
	$(LD) -o $@ $(OBJS) \
		--start-group $(LIBS) --end-group \
		$(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)
	rm -rf $(HEX_IMAGE)
	rm -f $(OBJS)
	rm -f $(PROJECT).lst

flash:
	openocd -f interface/stlink-v2.cfg \
		-f target/stm32f4x_stlink.cfg \
		-c "init" \
		-c "reset init" \
		-c "halt" \
		-c "flash write_image erase $(PROJECT).elf" \
		-c "verify_image $(PROJECT).elf" \
		-c "reset run" -c shutdown || \
	st-flash write $(BIN_IMAGE) 0x8000000

debug:
	openocd -f interface/stlink-v2.cfg \
		-f target/stm32f4x_stlink.cfg \
		-c "init" \
		-c "reset halt"

.PHONY: clean
