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

//assign cs_1 = (re | wr) & addr[20];

reg [3:0] state;
parameter ready = 4'b0001;
parameter setup = 4'b0010;
parameter pulse = 4'b0100;
parameter hold  = 4'b1000;

//SRAM controller captures.

always @ posedge (clk) begin
  case(state):

    //Incoming data write command
    ready: begin
    end

    setup: begin
      cs_1_sram <= ~addr_r[20];   //bottom portion of memory is in this chip
      cs_2_sram <= addr_r[20];   //top portion of memory is in this chip
      addr_sram <= addr_r;
      if (num_setup_cycles < SETUP_CYCLES) begin
        num_setup_cycles = num_setup_cycles + 1;
      end else begin
        state <= pulse;
      end
    end

    pulse: begin
    end

    hold: begin
    end

    default: begin
    end
  endcase
end


always @ (*) begin
  case(state):

    ready: begin
    end

    setup: begin
      sram_addr <= addr;
    end

    pulse: begin
      sram_addr <= addr;
    end

    hold: begin
      sram_addr <= addr;
    end

    default: begin
    end

  endcase


  if (rst) begin
    addr <= 0;
    data_in <= 0;
    data_out <= 0;
    re <= 0;
    wr <= 0;
    cs_1 <= 0;
    cs_2 <= 0;
  end
end


endmodule
