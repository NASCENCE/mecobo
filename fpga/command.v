module mecoCommand (clk, reset, ram_addr, ram_data_in, ram_data_out, ram_wr, ram_en);

input clk, reset;
output [20:0] ram_addr;
input [15:0] ram_data_in;
output [15:0] ram_data_out;
output ram_wr;
output ram_en;

localparam INSTRUCTION_ADDR 20'h42;

reg [15:0] read_data ;

assign ram_addr <= INSTRUCTION_ADDR;
assign ram_en <= 1'b1;
assign ram_wr <= 1'b0;

always @ (posedge clk) begin
  read_data <= ram_data_in;
end

//command is essentially a small multi-cycle processor, but the instruction is
//always at the same place.

// pinController X setup.
// 1 Read a instruction from the instruction register.
// 2 Read X bytes, depending on the instruction (instruciton includes which
// pin to configure). 

//each pinController has a slice of the memory map.
//the uC writes to that memory area to update the config...
//
//problem: internal memory in pin config VS the global memory in blockRAM.
// we can use distributed RAM for configuration data and
// block ram for sample data that is to be sent back.
// the EBI state machine then has to do more than what it 
// currently does. It can actually the the one responsible for
// everything...
//
// blah, this isn't really going anywhere.
//
// For now, the solution will be a dual port
// RAM, accessed by two state machines. 
//
// This module will read the instruction register,
// and then do whatever is needed. Usually that means
// programming a pin to some output function.
//
//
//Force 1-hot encoding.
/*
localparam [6:0] 
idle                     = 1;
fetch_instr              = 2;
decode_instr             = 4;
execute_instr            = 8;
program_pin              = 16;  //program 1 pin.
program_pin_fetch_data   = 32;  //fetch data  (X WORDS)
read_pin                 = 64;  //read 1 pin.

always @ (posedge clk) begin
  if (reset) 
    state <= idle;
  else
    state <= next_state;
end

always @ (*) begin
  next_state <= state;
  case(state) 
    idle: begin
      next_state <= fetch_instr;
      //defaults.
      addr <= INSTRUCTION_ADDR;
      write <= 1'b0;
      enable <= 1'b1;
    end

    fetch_instr: begin
      next_state <= execute_instr;
      addr <= INSTRUCTION_ADDR;
      write <= 1'b0;
      enable <= 1'b1;
    end

    execute_instr: begin
      next_state <= program_pin;
      addr <= INSTRUCTION_ADDR;
      write <= 1'b0;
      enable <= 1'b0;
    end
  endcase
end
*/

endmodule;
