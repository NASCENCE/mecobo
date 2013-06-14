module ebi_interface (clk, reset, data, addr, wr, rd, cs);
input clk;
input reset;
inout [15:0] data;
input [20:0] addr;

input wr;
input rd;
input cs;

inout [15:0] data;

//reg [15:0] data_in;
//reg [15:0] data_out;

wire [20:0] addr;
wire wr;
wire rd;
wire cs;

reg ram_enable;
reg ram_we;
wire [15:0] ram_data_in;
wire [15:0] ram_data_out;

assign data = (rd)? ram_data_out : 15'bZ;
assign ram_data_in = data;

sp_ram ram0 (
  .clk(clk),
  .en(ram_enable),
  .we(ram_we),
  .addr(addr[11:0]),
  .di(ram_data_in),
  .do(ram_data_out)
);

//EBI FSM
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
      if (cs && wr) begin
        next_state <= ebi_write_data;
        //Outputs from fsm
        ram_enable <= 1'b1; 
        ram_we <= 1'b1;
      end else begin 
        if (cs && rd) begin
          next_state <= ebi_read_data;
          ram_enable  <= 1'b1;
          ram_we <= 1'b0;
        end
      end
    end

    ebi_write_data: begin
      next_state <= idle;
      //Out
      ram_enable  <= 1'b1;
      ram_we <= 1'b1;
    end

    ebi_read_data : begin
      next_state <= idle;
      ram_enable  <= 1'b1;
    end
    default: begin
      next_state <= idle;
      //out
      ram_enable <= 0;
      ram_we <= 1'b0;
    end
  endcase
end


endmodule
