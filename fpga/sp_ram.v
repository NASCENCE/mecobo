module sp_ram (clk, en, we, addr, di, do);
input clk;
input en;
input we;
input [11:0] addr;
input [15:0] di;
output [15:0] do;

reg [15:0] RAM [15:0];
reg [15:0] do;

always @(posedge clk) begin
  if (en) begin
    if (we) 
      RAM[addr] <= di;
    do <= RAM[addr];
  end
end

endmodule
