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
  addr = 4;
  data_in = 2;
  #21
  addr = 8;
  data_in = 2;
  #21
  addr = 12;
  data_in = 5;
  #21
  addr = 16;
  data_in = 1;
  #21
  addr = 0;
  data_in = 1;
  #21
  data_in = 0;
end

reg [20:0] addr;
reg [15:0] data_in;
reg [15:0] data_out;
wire pin_out;
//DUT
pincontrol pc0 (
  .clk(clk),
  .reset(reset),
  .addr(addr),
  .data_in(data_in),
  .data_out(data_out),
  .pin_output(pin_out));
endmodule
