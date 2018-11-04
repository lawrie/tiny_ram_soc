FIRMWARE_DIR = ../../firmware
HDL_DIR = ../../hdl
INCLUDE_DIR = ../../libraries
VERILOG_FILES = \
           $(HDL_DIR)/top.v \
           $(HDL_DIR)/picosoc/uart/simpleuart.v \
           $(HDL_DIR)/picosoc/picosoc.v \
           $(HDL_DIR)/picorv32/picorv32.v \
           $(HDL_DIR)/picosoc/ili9341/ili9341_direct.v \
           $(HDL_DIR)/picosoc/gpio/gpio.v \
           $(HDL_DIR)/picosoc/sdcard/sdcard.v \
           $(HDL_DIR)/picosoc/spi_master/spi_master.v

PCF_FILE = $(HDL_DIR)/portable.pcf
LDS_FILE = $(FIRMWARE_DIR)/firmware.lds
START_FILE = $(FIRMWARE_DIR)/firmware.S
C_FILES = main.c $(INCLUDE_DIR)/uart/uart.c
DEFINES = -Dili9341_direct -Dgpio -Dsdcard

include $(HDL_DIR)/tiny_soc.mk

