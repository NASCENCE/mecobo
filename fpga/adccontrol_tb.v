`timescale 1ns/1ns
module adc_control_tb;

reg clk;
reg sclk;
reg rst;

wire ndc_cs;
wire adc_din;
wire adc_dout;
wire busy;

always #10 clk <= !clk;
always #200 sclk <= !sclk;

reg [15:0] adc_reg = 0;
reg [15:0] data_in = 0;
reg [15:0] data_out = 0;
reg [18:0] addr = 0;

wire adc_sclk;

initial #0 begin
 // $monitor("adc_dout: %b\n", adc_dout);
  sclk = 0;
  clk = 0;
  rst = 1;
  #220;
  rst = 0;
  addr = 1;
  data_in = 16'b1000010000000000;
  //data_in = $random;
  #820;  //This is two cycles. 
  addr = 0;   
  //#(17*400);
  $display("busy: %b\n", busy);
  wait (busy == 1'b0) #1;

//  $display("Input %h, got %h\n", data_in, adc_reg);
  addr = 1;
  data_in = 16'hFAAA;
  #400;
  data_in = 0;
  addr = 0;
  wait (busy == 1'b0) #1;
  #500;
  $display("Getting some data\n");
  addr = 2;
  #400;
  wait (busy == 1'b0) #1;
  $display("data_out: %h\n", data_out);
end

adc_dummy adc_d0 (
  .clk(sclk),
  .cs(adc_cs),
  .din(adc_din),
  .dout(adc_dout)
);

adc_control adc0 (
  .clk(clk),
  .sclk(sclk),
  .reset(rst),
  .busy(busy),
  .addr(addr),
  .data_in(data_in),
  .data_out(data_out),
  .cs(adc_cs),
  .adc_din(adc_din),
  .adc_sclk(adc_sclk),
  .adc_dout(adc_dout)
);

endmodule
