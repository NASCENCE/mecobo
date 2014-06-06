module ebi_interface (clk, reset, ebi_data, ebi_addr, ebi_wr, ebi_rd, ebi_cs,
                                  ram_data_in, ram_data_out, ram_addr, ram_wr, ram_en);
input clk;
input reset;

//From external world [uC]
inout [15:0] ebi_data; //NB! Inout.
input [20:0] ebi_addr;

input ebi_wr;
input ebi_rd;
input ebi_cs;

input [15:0]  ram_data_in;
output [15:0] ram_data_out;


output [20:0] ram_addr;
output ram_wr;
output ram_en;

//Drive EBI data with output from Block RAM if reading. (real tri state)
assign ebi_data = (ebi_rd)? ram_data_in : 16'bZ;
assign ram_data_out = ebi_data;  //Else, just drive data out with ebi data.
assign ram_addr = ebi_addr;

assign ram_wr = ebi_wr;
assign ram_en = ebi_cs;

//EBI FSM
/*
localparam [4:0]
  idle            = 5'b00001,
  ebi_read_data   = 5'b00010,
  ebi_write_data  = 5'b00100,
  ram_read_data   = 5'b01000,
  ram_write_data  = 5'b10000;

reg [4:0] state;
reg [4:0] next_state;

always @ (posedge clk) begin
  if (reset)
    state <= idle;
  else
    state <= next_state;
end

always @ (*) begin
  next_state <= state;
  case (state)
    idle: begin
      if (cs && wr)
        next_state <= ebi_write_data;
      else if (cs && rd)
        next_state <= ebi_read_data;
      else
        next_state <= idle;

      ram_enable <= 1'b0;
      ram_we <= 1'b0;

    end

    ebi_write_data: begin
      next_state <= idle;
      //Pulse this one cycle. 
      ram_enable  <= 1'b1;
      ram_we <= 1'b1;
    end

    ebi_read_data : begin
      next_state <= idle;
      ram_enable  <= 1'b1;
      ram_we <= 1'b0;
    end
    default: begin
      next_state <= idle;
      //out
      ram_enable <= 1'b0;
      ram_we <= 1'b0;
    end
  endcase
end
*/

endmodule
