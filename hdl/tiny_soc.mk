upload: hardware.bin firmware.bin
	tinyprog -p hardware.bin

hardware.blif: $(VERILOG_FILES) firmware.hex
	yosys -f "verilog $(DEFINES)" -ql hardware.log -p 'synth_ice40 -top top -blif hardware.blif' $(VERILOG_FILES)

hardware.asc: $(PCF_FILE) hardware.blif
	arachne-pnr -d 8k -P cm81 -o hardware.asc -p $(PCF_FILE) hardware.blif

hardware.bin: hardware.asc
	icetime -d hx8k -c 16 -mtr hardware.rpt hardware.asc
	icepack hardware.asc hardware.bin

firmware.elf: $(C_FILES) 
	/opt/riscv32i/bin/riscv32-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostartfiles -Wl,-Bstatic,-T,$(LDS_FILE),--strip-debug,-Map=firmware.map,--cref -fno-zero-initialized-in-bss -ffreestanding -nostdlib -o firmware.elf -I$(INCLUDE_DIR)  $(START_FILE) $(C_FILES)

firmware.bin: firmware.elf
	/opt/riscv32i/bin/riscv32-unknown-elf-objcopy -O binary firmware.elf /dev/stdout > firmware.bin

firmware.hex: $(FIRMWARE_DIR)/makehex.py firmware.bin
	python3 $(FIRMWARE_DIR)/makehex.py firmware.bin 3584 > firmware.hex
	@echo "Firmware size: $$(grep .. firmware.hex | wc -l) / $$(wc -l < firmware.hex)"

clean:
	rm -f firmware.elf firmware.hex firmware.bin firmware.o firmware.map \
	      hardware.blif hardware.log hardware.asc hardware.rpt hardware.bin

