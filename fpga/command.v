module mecoCommand (clk, reset, ram_addr, ram_data_in, ram_data_out, ram_wr, ram_en, pin_out);

input clk, reset;
output [20:0] ram_addr;
input [15:0] ram_data_in;
output [15:0] ram_data_out;
output ram_wr;
output ram_en;
output [15:0] pin_out;

localparam INSTRUCTION_ADDR = 20'hF;

reg [15:0] read_data;

assign ram_addr = INSTRUCTION_ADDR;
assign ram_en = 1'b1;
assign ram_wr = 1'b0;

//Force 1-hot encoding.
localparam [2:0] 
  idle = 3'b001,
  get  = 3'b010;
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

endmodule
