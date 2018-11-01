/*
 *  PicoSoC - A simple example SoC using PicoRV32
 *
 *  Copyright (C) 2017  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

module top (
    input CLK,

    // hardware UART
    output SER_TX,
    input SER_RX,

`ifdef flash_write
    inout SPI_IO1,
    inout SPI_SS,
    inout SPI_IO0,
    inout SPI_SCK,
`endif

`ifdef pdm_audio
    // Audio out pin
		output AUDIO_RIGHT,
    output AUDIO_LEFT,
`endif

`ifdef i2c
    // I2C pins
    inout I2C_SDA,
    inout I2C_SCL,
`endif

`ifdef oled
    inout OLED_SPI_SCL,
    inout OLED_SPI_SDA,
    inout OLED_SPI_RES,
    inout OLED_SPI_DC,
    inout OLED_SPI_CS,
`endif

`ifdef sdcard
    inout SD_MISO,
    inout SD_MOSI,
    inout SD_SCK,
    inout SD_CS,
`endif

`ifdef ili9341_direct
        output lcd_D0,
	output lcd_D1,
	output lcd_D2,
	output lcd_D3,
	output lcd_D4,
	output lcd_D5,
	output lcd_D6,
	output lcd_D7,
	output lcd_nreset,
        output lcd_cmd_data,
        output lcd_write_edge,
        output lcd_backlight,
`endif

`ifdef ili9341
        output lcd_D0,
	output lcd_D1,
	output lcd_D2,
	output lcd_D3,
	output lcd_D4,
	output lcd_D5,
	output lcd_D6,
	output lcd_D7,
	output lcd_nreset,
        output lcd_cmd_data,
        output lcd_write_edge,
        output lcd_backlight,
`elsif vga
    output VGA_VSYNC,
    output VGA_HSYNC,
    output VGA_R,
    output VGA_G,
    output VGA_B,
`endif

`ifdef gpio
    // GPIO buttons
    inout  [7:0] BUTTONS,

    // onboard LED
    output LED,
`endif

    // onboard USB interface
    output USBPU
);
    // Disable USB
    assign USBPU = 1'b0;

    ///////////////////////////////////
    // Power-on Reset
    ///////////////////////////////////
    reg [5:0] reset_cnt = 0;
    wire resetn = &reset_cnt;

    always @(posedge CLK) begin
        reset_cnt <= reset_cnt + !resetn;
    end

    ///////////////////////////////////
    // Peripheral Bus
    ///////////////////////////////////
    wire        iomem_valid;
    wire        iomem_ready;

    wire [3:0]  iomem_wstrb;
    wire [31:0] iomem_addr;
    wire [31:0] iomem_wdata;
    wire [31:0] iomem_rdata;

    // enable signals for each of the peripherals
    wire gpio_en   = (iomem_addr[31:24] == 8'h03); /* GPIO mapped to 0x03xx_xxxx */
    wire audio_en  = (iomem_addr[31:24] == 8'h04); /* Audio device mapped to 0x04xx_xxxx */
    wire video_en  = (iomem_addr[31:24] == 8'h05); /* Video device mapped to 0x05xx_xxxx */
    wire sdcard_en  = (iomem_addr[31:24] == 8'h06); /* SPI SD card mapped to 0x06xx_xxxx */
    wire i2c_en    = (iomem_addr[31:24] == 8'h07); /* I2C device mapped to 0x07xx_xxxx */
    wire flash_en    = (iomem_addr[31:24] == 8'h08); /* flash memory SPI mapped to 0x08xx_xxxx */

`ifdef warm_boot
  wire boot_en    = (iomem_addr[31:24] == 8'h09); /* warm boot */
  
  SB_WARMBOOT warmboot_inst (
    .S1(1'b0),
    .S0(1'b1),
    .BOOT(boot_en)
  );
`endif

`ifdef pdm_audio
    wire audio_data;
  	assign AUDIO_LEFT = audio_data;
  	assign AUDIO_RIGHT = audio_data;
  	audio audio_peripheral(
  		.clk(CLK),
  		.resetn(resetn),
  		.audio_out(audio_data),
  		.iomem_valid(iomem_valid && audio_en),
  		.iomem_wstrb(iomem_wstrb),
  		.iomem_addr(iomem_addr),
  		.iomem_wdata(iomem_wdata)
  );
`endif

  wire oled_iomem_ready;

`ifdef oled
  video_oled oled (
    .clk(CLK),
    .resetn(resetn),
    .iomem_valid(iomem_valid && video_en),
    .iomem_wstrb(iomem_wstrb),
    .iomem_addr(iomem_addr),
    .iomem_wdata(iomem_wdata),
    .iomem_ready(oled_iomem_ready),
    .OLED_SPI_SDA(OLED_SPI_SDA),
    .OLED_SPI_SCL(OLED_SPI_SCL),
    .OLED_SPI_CS(OLED_SPI_CS),
    .OLED_SPI_DC(OLED_SPI_DC),
    .OLED_SPI_RES(OLED_SPI_RES)
  );
`endif

  wire flash_iomem_ready;

`ifdef flash_write
  wire [31:0] flash_iomem_rdata;
  
  flash_write flash (
    .clk(CLK),
    .resetn(resetn),
    .iomem_valid(iomem_valid && flash_en),
    .iomem_wstrb(iomem_wstrb),
    .iomem_addr(iomem_addr),
    .iomem_wdata(iomem_wdata),
    .iomem_rdata(flash_iomem_rdata),
    .iomem_ready(flash_iomem_ready),
    .SPI_MISO(SPI_IO1),
    .SPI_SCK(SPI_SCK),
    .SPI_CS(SPI_SS),
    .SPI_MOSI(SPI_IO0)
  );
`endif

  wire ili_direct_iomem_ready;

`ifdef ili9341_direct

      assign lcd_backlight = 1;

      ili9341_direct lcd_peripheral(
                .clk(CLK),
                .resetn(resetn),
                .iomem_valid(iomem_valid && video_en),
                .iomem_wstrb(iomem_wstrb),
                .iomem_addr(iomem_addr),
                .iomem_wdata(iomem_wdata),
                .iomem_ready(ili_direct_iomem_ready),
                .nreset(lcd_nreset),
                .cmd_data(lcd_cmd_data),
                .write_edge(lcd_write_edge),
                .dout({lcd_D7, lcd_D6, lcd_D5, lcd_D4,
                       lcd_D3, lcd_D2, lcd_D1, lcd_D0}));
`endif

`ifdef sdcard
  wire [31:0] sdcard_iomem_rdata;

  sdcard sd (
    .clk(CLK),
    .resetn(resetn),
    .iomem_valid(iomem_valid && sdcard_en),
    .iomem_wstrb(iomem_wstrb),
    .iomem_addr(iomem_addr),
    .iomem_wdata(iomem_wdata),
    .iomem_rdata(sdcard_iomem_rdata),
    .iomem_ready(sdcard_iomem_ready),
    .SD_MOSI(SD_MOSI),
    .SD_MISO(SD_MISO),
    .SD_SCK(SD_SCK),
    .SD_CS(SD_CS)
  );
`endif

  wire sdcard_iomem_ready;

`ifdef ili9341

      assign lcd_backlight = 1;

      video_vga vga_video_peripheral(
      		.clk(CLK),
      		.resetn(resetn),
      		.iomem_valid(iomem_valid && video_en),
      		.iomem_wstrb(iomem_wstrb),
      		.iomem_addr(iomem_addr),
      		.iomem_wdata(iomem_wdata),
                .nreset(lcd_nreset),
                .cmd_data(lcd_cmd_data),
		.write_edge(lcd_write_edge),
		.dout({lcd_D7, lcd_D6, lcd_D5, lcd_D4,
		       lcd_D3, lcd_D2, lcd_D1, lcd_D0})
      );
`elsif vga
      video_vga vga_video_peripheral(
      		.clk(CLK),
      		.resetn(resetn),
      		.iomem_valid(iomem_valid && video_en),
      		.iomem_wstrb(iomem_wstrb),
      		.iomem_addr(iomem_addr),
      		.iomem_wdata(iomem_wdata),
      		.vga_hsync(VGA_HSYNC),
      		.vga_vsync(VGA_VSYNC),
      		.vga_r(VGA_R),
      		.vga_g(VGA_G),
      		.vga_b(VGA_B)
      	);
`endif

  wire [31:0] gpio_iomem_rdata;
  wire gpio_iomem_ready;

`ifdef gpio
  gpio gpio_peripheral(
    .clk(CLK),
    .resetn(resetn),
    .iomem_ready(gpio_iomem_ready),
    .iomem_rdata(gpio_iomem_rdata),
    .iomem_valid(iomem_valid && gpio_en),
    .iomem_wstrb(iomem_wstrb),
    .iomem_addr(iomem_addr),
    .iomem_wdata(iomem_wdata),
    .BUTTONS(BUTTONS),
    .led(LED)
  );
`else
  assign gpio_iomem_ready = gpio_en;
  assign gpio_iomem_rdata = 32'h0;
`endif


///////////////////////////
// Controller Peripheral
///////////////////////////

wire [31:0] i2c_iomem_rdata;
wire i2c_iomem_ready;

`ifdef i2c
  i2c i2c_peripheral(
    .clk(CLK),
    .resetn(resetn),
    .iomem_ready(i2c_iomem_ready),
    .iomem_rdata(i2c_iomem_rdata),
    .iomem_valid(iomem_valid && i2c_en),
    .iomem_wstrb(iomem_wstrb),
    .iomem_addr(iomem_addr),
    .iomem_wdata(iomem_wdata),
    .I2C_SCL(I2C_SCL),
    .I2C_SDA(I2C_SDA)
  );
`else
  assign i2c_iomem_ready = i2c_en;     /* if i2c peripheral is "disconnected", fake always available zero bytes */
  assign i2c_iomem_rdata = 32'h0;
`endif


assign iomem_ready = i2c_en ? i2c_iomem_ready : gpio_en ? gpio_iomem_ready 
`ifdef oled
                     : video_en ? oled_iomem_ready
`endif
`ifdef ili9341_direct
                     : video_en ? ili_direct_iomem_ready
`endif
`ifdef sdcard
                     : sdcard_en ? sdcard_iomem_ready
`endif
`ifdef flash_write
                     : flash_en ? flash_iomem_ready
`endif
                     : 1'b1;

assign iomem_rdata =  i2c_iomem_ready ? i2c_iomem_rdata
                    : gpio_iomem_ready ? gpio_iomem_rdata
`ifdef sdcard
                    : sdcard_iomem_ready ? sdcard_iomem_rdata
`endif
`ifdef flash_write
                    : flash_iomem_ready ? flash_iomem_rdata
`endif
                    : 32'h0;

picosoc #(
	.BARREL_SHIFTER(0),
	.ENABLE_MULDIV(0),
	.ENABLE_COMPRESSED(0),
	.ENABLE_COUNTERS(0),
	.ENABLE_IRQ_QREGS(1),
	.ENABLE_TWO_STAGE_SHIFT(0),
	.PROGADDR_RESET(32'h0005_0000), // beginning of user space in SPI flash
	.PROGADDR_IRQ(32'h0005_0010),
	.MEM_WORDS(3584),                // use 4KBytes of block RAM by default (8 RAMS)
	.STACKADDR(3584),   /* stack addr = byte offset; stack starts at 0x400, grows downward. Data starts at 0x400+. */
	.ENABLE_IRQ(1)
) soc (
	.clk          (CLK         ),
	.resetn       (resetn      ),

	.ser_tx       (SER_TX      ),
	.ser_rx       (SER_RX      ),

	.irq_5        (1'b0),
	.irq_6        (1'b0        ),
	.irq_7        (1'b0        ),

	.iomem_valid  (iomem_valid ),
	.iomem_ready  (iomem_ready ),
	.iomem_wstrb  (iomem_wstrb ),
	.iomem_addr   (iomem_addr  ),
	.iomem_wdata  (iomem_wdata ),
	.iomem_rdata  (iomem_rdata )
);
endmodule
