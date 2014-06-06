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
reg [15:0] data_to_adc = 0;
reg [15:0] data_from_adc;
reg [18:0] addr = 0;

//Dump to file.
wire adc_sclk;
 initial  begin
  $dumpfile ("counter.vcd"); 
  $dumpvars; 
end 


initial #0 begin
  sclk = 0;
  clk = 0;
  rst = 1;
  #220;
  //Program the AD controller to output channel 1
  rst = 0;
  addr =    16'h0000;
  data_to_adc = 16'b1000010000000011;
  #820;  //This is two cycles. 
  data_to_adc = 0; //reset data in after a cycle or two.
  $display("busy: %b\n", busy);
  wait (busy == 1'b0) #1;

  //Now we'll get a sample from channel 1. We don't need read/write, it's well-defined.
  addr = 16'h1300;
  #(200*17) //one cycle wait for data to be clocked in....
  $display("data_from_adc: %h\n", data_from_adc);
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
  .data_in(data_to_adc),
  .data_out(data_from_adc),
  .cs(adc_cs),
  .adc_din(adc_din),
  .adc_sclk(adc_sclk),
  .adc_dout(adc_dout)
);

endmodule
