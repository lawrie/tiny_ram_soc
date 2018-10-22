
module ili9341_direct
(
  input            resetn,
  input            clk,
  input            iomem_valid,
  output reg       iomem_ready,
  input [3:0]      iomem_wstrb,
  input [31:0]     iomem_addr,
  input [31:0]     iomem_wdata,
  output [31:0]    iomem_rdata,
  output reg       nreset,
  output reg       cmd_data, // 1 => Data, 0 => Command
  output reg       write_edge, // Write signal on rising edge
  output reg [7:0] dout);

  reg [1:0] state = 0;
  reg [2:0] fast_state;
  reg [15:0] num_bytes;

  always @(posedge clk) begin
    iomem_ready <= 0;
    if (!resetn) begin
      state <= 0;
      fast_state <= 0;
      cmd_data <= 0;
      nreset <= 1;
      write_edge <= 0;
    end else if (iomem_valid && !iomem_ready) begin
      if (iomem_wstrb) begin
        iomem_ready <= 1;
        if (iomem_addr[7:0] == 'h08) cmd_data <= iomem_wdata;
        else if (iomem_addr[7:0] == 'h0c) nreset <= iomem_wdata;
        else if (iomem_addr[7:0] == 'h00) begin
          case (state)
            0 : begin
              write_edge <= 0;
              dout <= iomem_wdata[7:0];
              state <= 1;
              iomem_ready <= 0;
            end
            1 : begin
               write_edge <= 1;
               state <= 2;
               iomem_ready <= 0;
            end
            2 : begin
               write_edge <= 0;
               iomem_ready <= 1;
               state <= 0;
            end
          endcase
        end else if (iomem_addr[7:0] == 'h04) begin // fast xfer
          iomem_ready <= 0;
          case (fast_state)
            0 : begin
              num_bytes <= iomem_wdata[31:16];
              fast_state <= 1;
            end
            1: begin
              write_edge <= 0;
              dout <= iomem_wdata[15:8];
              fast_state <= 2;
            end
            2 : begin
               write_edge <= 1;
               fast_state <= 3;
            end
            3 : begin
               write_edge <= 0;
               dout <= iomem_wdata[7:0];
               fast_state <= 4;
            end
            4: begin
               write_edge <= 1;
               if (num_bytes == 1) begin
                 fast_state <= 5;
               end else begin
                 num_bytes <= num_bytes - 1;
                 fast_state <= 1;
              end
            end
            5: begin
               iomem_ready <= 1;
               write_edge <= 0;
               fast_state <= 0;
            end
          endcase
        end
      end
    end
  end

endmodule
