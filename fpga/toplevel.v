//16 bits words. 
//Each address addresses 1 16 bit word.
//Mecobo pin configuartion is done from address 0x32.
//Each pin has 6 words:
//[waveform] 1: square, 2: sawtooth 3: triangle 4: pwm (pointer)
//[freq]
//[phase]
//[ticks]
//[currentTick] -- read only
//[lastValue]


module mecobo (osc, reset, led, ebi_data, ebi_addr, ebi_wr, ebi_rd, ebi_cs, fpga_ready, HN, HW);

input osc;
input reset;

inout [15:0] ebi_data;
input [18:0] ebi_addr;
input ebi_wr;
input ebi_rd;
input ebi_cs;

//output [15:0] debug;

output [3:0] led;
output fpga_ready;
inout [60:1] HW;
inout [57:1] HN;


assign led[1] =  read_enable;
//Invert control signals
wire read_enable = !ebi_rd;
wire write_enable = !ebi_wr;
wire chip_select = !ebi_cs;


//(* tristate2logic = "yes" *)
wor [15:0] data_out;
wire [15:0] data_in;

assign data_in = ebi_data; //write op
//build multiplexer
assign ebi_data = (chip_select & read_enable) ? data_out : 16'bz; //read op

assign fpga_ready = 1'b1;

//Heartbeat
reg [26:0] led_heartbeat = 0;
assign led[0] = led_heartbeat[26] | led_heartbeat[25];

always @ (posedge sys_clk) begin
  if(reset)
    led_heartbeat <= 0;
  else
    led_heartbeat <= led_heartbeat + 1'b1;
end

wire adc_din;
wire adc_dout;
wire adc_cs;

wire sys_clk;
wire xbar_clk;
wire da_clk;
wire ad_clk;

wire xbar_predivided;

//----------------------------------- CLOCKING -------------------------
//DCM instance for clock division
main_clocks clocks
(
  .CLK_IN_50(osc),
  .CLK_OUT_75(sys_clk),
  .CLK_OUT_5(xbar_predivided),
  .CLK_OUT_10(ad_clk),
  .CLK_OUT_30(da_clk),
  .RESET(reset),
  .LOCKED(led[2])
);


xbar_clock(
  .CLK_IN_5(xbar_predivided),
  .XBAR_CLK(xbar_clk),
  .RESET(reset),
  .LOCKED(led[3])
);

ODDR2 clkout_oddr_ad
 (.Q  (HN[4]),
  .C0 (ad_clk),
  .C1 (~ad_clk),
  .CE (1'b1),
  .D0 (1'b1),
  .D1 (1'b0),
  .R  (1'b0),
  .S  (1'b0));

ODDR2 clkout_oddr_da
 (.Q  (HN[2]),
  .C0 (da_clk),
  .C1 (~da_clk),
  .CE (1'b1),
  .D0 (1'b1),
  .D1 (1'b0),
  .R  (1'b0),
  .S  (1'b0));


/*
wire [5:1] clk_int;
wire [5:1] clk_n;
wire [5:1] clk;
wire [5:1] ce;
wire [5:1] clk_out;

clockgen clknetwork
   (// Clock in ports
    .CLK_IN1            (osc),
    // Clock out ports
    .SYSCLK           (clk_int[1]),
    .XBAR           (clk_int[2]),
    .AD           (clk_int[3]),
    .DA           (clk_int[4]),
    .FAST           (clk_int[5]),
    // Status and control signals
    .RESET              (reset),
    .LOCKED             (led[2]));

genvar clk_out_pins;

generate 
  for (clk_out_pins = 1; clk_out_pins <= 5; clk_out_pins = clk_out_pins + 1) 
  begin: gen_outclk_oddr
  assign clk_n[clk_out_pins] = ~clk[clk_out_pins];

  ODDR2 clkout_oddr
   (.Q  (clk_out[clk_out_pins]),
    .C0 (clk[clk_out_pins]),
    .C1 (clk_n[clk_out_pins]),
    .CE (ce[clk_out_pins]),
    .D0 (1'b1),
    .D1 (1'b0),
    .R  (1'b0),
    .S  (1'b0));
  end
endgenerate

assign clk[1] = clk_int[1];
assign clk[2] = ~clk_int[2];
assign clk[3] = clk_int[3];
assign clk[4] = clk_int[4];
assign clk[5] = clk_int[5];

//Enable all by default some clocks.
assign ce[1] = 1'b1; //always on.
//XBar clock needs to be toggleable. We'll invert it so that we know that the
//flank goes to the XBAR after we've set up the data line.
//assign HN[6] = clk_out[2];
wire xbar_clock_enable; //from xbar control
assign ce[2] = xbar_clock_enable;

//clocks going out.
assign HN[4] = clk_out[3]; //ad_clk;
assign ce[3] = 1'b1; 
assign HN[2] = clk_out[4]; //da_clk;
assign ce[4] = 1'b1; 
*/



//Standard pin controllers
genvar i;
generate
  for (i = 1; i < 50; i = i + 1) begin: pinControl 
    if ((i != 29) && (i != 9) && (i != 11) && (i != 17) && (i != 19) && (i != 21) && (i != 23) && (i!=12) && (i!=20) && (i != 49)) 
    begin
      pincontrol #(.POSITION(i-1))
      pc (
        .clk(sys_clk),
        .reset(reset),
        .enable(chip_select),
        .addr(ebi_addr),
        .data_wr(write_enable),
        .data_in(data_in),
        .data_rd(read_enable),
        .data_out(data_out),
        .pin(HW[i])
      );
    end
  end
endgenerate

adc_control #(.POSITION(100))
    adc0 (
      .clk(sys_clk),
      .sclk(ad_clk),
      .reset(reset),
      .enable(chip_select),
      .re(read_enable),
      .wr(write_enable),
      .addr(ebi_addr),
      .data_in(data_in),
      .data_out(data_out),
      //interface to the world
      .cs(HN[48]),
      .adc_din(HN[32]),
      .adc_dout(HN[40]));

dac_control #(.POSITION(50))
    dac0 (
      .ebi_clk(sys_clk),
      .sclk(da_clk),
      .reset(reset),
      .enable(chip_select),
      .re(read_enable),
      .wr(write_enable),
      .data(data_in),
      .out_data(data_out),
      .addr(ebi_addr),
      .nLdac(HN[24]),
      .nSync(HN[16]),
      .dac_din(HN[8]));

xbar_control #(.POSITION(200))
    xbar0 (
      .ebi_clk(sys_clk),
      .sclk(xbar_clk),
      .reset(reset),
      .enable(chip_select),
      .re(read_enable),
      .wr(write_enable),
      .data_out(data_out),
      .data(data_in),
      .addr(ebi_addr),
      .xbar_clock(HN[6]), //clock from xbar and out to device 
      .pclk(HN[1]),
      .sin(HN[9]));

endmodule
