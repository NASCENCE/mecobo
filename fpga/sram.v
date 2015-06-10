module sram (

  //Internal interface for reading and writing data
  input clk,
  input rst,

  //The SRAM module can only handle one request at a time,
  //so while the state machine is busy servicing
  //the last one, nobody should bother it with other stuff.
  output busy,

  input   [20:0]  addr,
  input   [15:0]  data_in,
  output  [15:0]  data_out,

  input re,
  input wr,

  //interface towards chips
  inout [15:0] data_sram;
  output [19:0] addr_sram;
  output cs_1_sram,
  output cs_2_sram,
  output re_sram,
  output wr_sram

);


parameter SETUP_CYCLES = 1;
parameter PULSE_CYCLES = 1;
parameter HOLD_CYCLES = 1;

reg [3:0] num_setup_cycles = 0;
reg [3:0] num_pulse_cycles = 0;
reg [3:0] num_hold_cycles  = 0;

//This is the interface to the SRAM chips. They have 20 address lines,
//addressing 2MB of data, but organized in 16 bit words, giving 1MWords.
//
//addr[20] is used to select between chips, meaning that addr 
//0x0000 0000 -> 0x000F FFFF addresses SRAM 1
//0x0010 0000 -> 0x001F FFFF addresses SRAM 2. 


//reset signal for command reg
reg         cmd_reset;
//command capture
reg [20:0]  cmd_addr;
reg         cmd_r;
reg         cmd_w;
reg [15:0]  cmd_data;


//Capture incoming command.
always @ (posedge clk) begin
  if (!cmd_reset & !busy) begin
    cmd_addr <= addr;
    cmd_data <= data_in;
    cmd_r    <= re;
    cmd_w    <= wr;
  end else begin
    cmd_addr <= 0;
    cmd_data <= 0;
    cmd_r    <= 0;
    cmd_w    <= 0;
  end

  //Capture data from SRAM since we can expect it to be there now.
  if (capture_data) begin
    data_out <= data_sram;
end

//If it's a write command, just put data in output pins immediately.
assign data_sram = cmd_w ? cmd_data : 16'bZ;


reg [3:0] state;
parameter ready = 4'b0001;
parameter setup = 4'b0010;
parameter pulse = 4'b0100;
parameter hold  = 4'b1000;

reg reset_counters = 0;

always @ posedge (clk) begin
  case(state):

    //Incoming data write command
    ready: begin
      reset_counters <= 1'b0;
      if (cmd_r | cmd_w) begin
        state <= setup;
        busy <= 1'b1;
      end
    end

    setup: begin
      cs_1_sram <= ~cmd_addr[20];   //bottom portion of memory is in this chip
      cs_2_sram <= cmd_addr[20];   //top portion of memory is in this chip
      addr_sram <= cmd_addr[19:0];
      re <= cmd_r;
      wr <= cmd_w;

      if (num_setup_cycles < SETUP_CYCLES) begin
        num_setup_cycles = num_setup_cycles + 1;
      end else begin
        state <= pulse;
      end
    end

    pulse: begin
      //Pulse.
      cs_1_sram <= ~cmd_addr[20];   //bottom portion of memory is in this chip
      cs_2_sram <= cmd_addr[20];   //top portion of memory is in this chip
      addr_sram <= cmd_addr[19:0];
      re <= cmd_r;
      wr <= cmd_w;

      if (num_setup_cycles < SETUP_CYCLES) begin
        num_setup_cycles = num_setup_cycles + 1;
      end else begin
        state <= pulse;
      end
      state <= hold;
    end

    //Hold phase captures data if we're reading.
    hold: begin
      if (cmd_r) begin
        capture_data <= 1'b1;
      else
        reset_counters <= 1'b1;
    end

    default: begin
    end
  endcase
end


endmodule
