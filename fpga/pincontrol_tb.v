module pincontrol_tb;

reg clk, reset;

always begin
  #10 clk = !clk;
end

initial begin
  clk  = 0;
  reset = 1;

  #35
  reset = 0;
end

wire [20:0] addr;
wire [15:0] data_in;
wire [15:0] data_out;
wire done;
//DUT
pincontrol pc0 (
  .clk(clk),
  .reset(reset),
  .addr(addr),
  .data_in(data_in),
  .data_out(data_out),
  .done(done));
endmodule
