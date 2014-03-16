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


module mecobo (clk, reset, led, ebi_data, ebi_addr, ebi_wr, ebi_rd, ebi_cs, fpga_ready, HN, HW);

input clk;
input reset;

inout [15:0] ebi_data;
input [20:0] ebi_addr;
input ebi_wr;
input ebi_rd;
input ebi_cs;

//output [15:0] debug;

output [3:0] led;
output fpga_ready;
inout [60:0] HW;
inout [57:0] HN;

assign fpga_ready = 1'b0;

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

reg [23:0] led_heartbeat = 0;
assign led = led_heartbeat[22] | led_heartbeat[23] | led_heartbeat[21];

always @ (posedge sys_clk) begin
  if(reset)
    led_heartbeat <= 0;
  else
    led_heartbeat[23:0] <= led_heartbeat + 1'b1;
end

wire adc_din;
wire adc_dout;
wire adc_cs;

//----------------------------------- CLOCKING -------------------------
//DCM instance for clock division
wire sys_clk;
wire xbar_clk;
wire da_clk;
wire ad_clk;


assign HN[4] = ad_clk;
assign HN[2] = da_clk;

clockgen dcm0
(
  .CLK_IN1(clk), //50Mhz (IN FROM OSC)
  .SYSCLK(sys_clk), //100MHz
  .XBAR(xbar_clk), //5Mhz
  .AD(ad_clk),  //10Mhz
  .DA(da_clk), //30MHz
  .FAST(), //unused, 200MHz
  .RESET(reset),
  .LOCKED(led[2])
);

//Standard pin controllers
genvar i;
generate
  for (i = 0; i < 60; i = i + 1) begin: pinControl 
    if ((i != 29)) begin
      pincontrol #(.POSITION(i))
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

adc_control #(.POSITION(61))
    adc0 (
      .clk(sys_clk),
      .sclk(ad_clk),
      .reset(reset),
      .busy(),
      .re(ebi_rd),
      .wr(ebi_wr),
      .addr(ebi_addr),
      .data_in(data_in),
      .data_out(data_out),
      //interface to the world
      .cs(HN[48]),
      .adc_din(HN[40]),
      .adc_dout(HN[32]));

dac_control #(.POSITION(62)) 
    dac0 (
      .ebi_clk(sys_clk),
      .sclk(da_clk),
      .reset(reset),
      .re(ebi_rd),
      .wr(ebi_wr),
      .data(ebi_data),
      .addr(ebi_addr),
      .nLdac(HN[8]),
      .nSync(HN[16]),
      .dac_din(HN[24]));
xbar_control #(.POSITION(63)) 
    xbar0 (
      .ebi_clk(sys_clk),
      .sclk(xbar_clk),
      .reset(reset),
      .re(ebi_rd),
      .wr(ebi_wr),
      .data(ebi_data),
      .addr(ebi_addr),
      .xbar_clock(HN[6]), //clock from xbar and out to device (BUFCE in module)
      .pclk(HN[1]),
      .sin(HN[17]));
endgenerate

endmodule
