`timescale 1ns/1ns
module adc_control_tb;

reg clk;
reg sclk;
reg rst;

wire adc_cs;
wire adc_din;
wire busy;

always #10 clk <= !clk;
always #200 sclk <= !sclk;

reg [15:0] adc_reg = 0;
reg [15:0] data_in = 0;
reg [18:0] addr = 0;

wire adc_dout;
wire adc_sclk;

initial #0 begin
 // $monitor("state: %h\n", adc0.state);
  sclk = 0;
  clk = 0;
  rst = 1;
  #220;
  rst = 0;
  addr = 1;
  //data_in = 16'hDEAD;
  data_in = $random;
  #820;  //This is two cycles. 
  addr = 0;   
  //#(17*400);
  $display("busy: %b\n", busy);
  wait (busy == 1'b0) #1;

  $display("Input %h, got %h\n", data_in, adc_reg);
  addr = 1;
  data_in = 16'hAAAA;
end

always @ (posedge sclk)
begin
  if (adc_cs) begin
    adc_reg <= adc_reg << 1;
    adc_reg[0] <= adc_din;
  end
end

task send_word_check_result (
  input [18:0] addr,
  input [15:0] word);
begin
end
endtask
//  $display("cs: %d data:", ,cs);

adc_control adc0 (
  .clk(clk),
  .sclk(sclk),
  .reset(rst),
  .busy(busy),
  .addr(addr),
  .data_in(data_in),
  .data_out(),
  .cs(adc_cs),
  .adc_din(adc_din),
  .adc_sclk(adc_sclk),
  .adc_dout()
);

endmodule
