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



`define WITH_DB

module mecobo   
(               osc, 
reset, 
led, 
ebi_data, 
ebi_addr, 
ebi_wr, 
ebi_rd, 
ebi_cs, 
fpga_ready, 
HN, 
HW
);

input osc;
input reset;

inout [15:0] ebi_data;
input [18:0] ebi_addr;
input ebi_wr;
input ebi_rd;
input ebi_cs;

//SRAM INTERFACE
//output  sram_addr;
//inout   sram_data;
//output  sram_rd;
//output  sram_wr;
//output  sram_cs[2:0];

//output [15:0] debug;

output [3:0] led;
output fpga_ready;
inout [60:1] HW;
inout [57:1] HN;

//Invert control signals
wire read_enable = !ebi_rd;
wire write_enable = !ebi_wr;
wire chip_select = !ebi_cs;

`ifndef WITH_DB
assign led[3] =  1'b0;
`endif
assign led[1] =  read_enable;
//assign led[3] = chip_select;


//(* tristate2logic = "yes" *)
wor [15:0] data_out;
wire [15:0] data_in;

assign data_in = ebi_data; //write op
//build multiplexer
assign ebi_data = (chip_select & read_enable) ? data_out : 16'bz; //read op

wire ebi_irq;
assign fpga_ready = ebi_irq;

//Heartbeat
reg [26:0] led_heartbeat = 0;
assign led[0] = led_heartbeat[26] | led_heartbeat[25];

wire adc_din;
wire adc_dout;
wire adc_cs;

wire sys_clk;
wire xbar_clk;
wire da_clk;
wire ad_clk;

wire xbar_predivided;


wire sample_enable_output;
wire [7:0] sample_channel_select;
wire [31:0] sample_data_bus;

wire locked;
assign led[2] = locked;

//                     locked   reset
// 0                   1        0 
// 1                   0        1
// 0                   1        1
// 1                   0        0
wire mecobo_reset = ~locked & ~reset;


//----------------------------------- CLOCKING -------------------------
/* heartbeat */
always @ (posedge sys_clk) begin
  if(mecobo_reset)
    led_heartbeat <= 0;
  else
    led_heartbeat <= led_heartbeat + 1'b1;
end


//DCM instance for clock division
main_clocks clocks
(
  .CLK_IN_50(osc),
  .CLK_OUT_75(sys_clk),
  .CLK_OUT_5(xbar_predivided),
  .CLK_OUT_10(ad_clk),
  .CLK_OUT_30(da_clk),
  .RESET(1'b0),
  .LOCKED(locked)
);


`ifdef WITH_DB
  xbar_clock xbarclocks0(
    .CLK_IN_5(xbar_predivided),
    .XBAR_CLK(xbar_clk),
    .RESET(1'b0),
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
`endif

wire [31:0] global_clock;
wire [79:0] ebi_fifo_din;
wire [15:0] sample_collector_data;
wire [15:0] sample_fifo_data_count;
wire [9:0]  cmd_fifo_data_count;
//EBI
ebi ebi_if(
  .clk(sys_clk),
  .rst(mecobo_reset),
  .data_in(data_in),
  .data_out(data_out),
  .addr(ebi_addr),
  .rd(read_enable),
  .wr(write_enable),
  .cs(chip_select),
  .global_clock(global_clock),
  .cmd_fifo_data_in(ebi_fifo_din),
  .cmd_fifo_wr_en(ebi_fifo_wr),
  .cmd_fifo_almost_full(ebi_fifo_almost_full),
  .cmd_fifo_full(ebi_fifo_full),
  .cmd_fifo_almost_empty(ebi_fifo_almost_empty),
  .cmd_fifo_empty(ebi_fifo_empty),
  .cmd_fifo_data_count(cmd_fifo_data_count),
  .sample_fifo_data_out(sample_collector_data),
  .sample_fifo_rd_en(sample_collector_rd_en),
  .sample_fifo_empty(sample_fifo_empty),
  .sample_fifo_almost_empty(sample_fifo_almost_empty),
  .sample_fifo_full(sample_fifo_full),
  .sample_fifo_almost_full(sample_fifo_almost_full),
  .sample_fifo_data_count(sample_fifo_data_count),
  .irq(ebi_irq)
);


//FIFOS
wire [31:0] cmd_bus_data_in;
wire [15:0] cmd_bus_addr;
wire [79:0] sched_fifo_data;
command_fifo cmd_fifo (
  .clk(sys_clk),
  .rst(mecobo_reset),
  .din(ebi_fifo_din),
  .wr_en(ebi_fifo_wr),
  .full(ebi_fifo_full),
  .almost_full(ebi_fifo_almost_full),
  .empty(ebi_fifo_empty),
  .almost_empty(ebi_fifo_almost_empty),
  .valid(sched_fifo_valid),
  .dout(sched_fifo_data),
  .rd_en(sched_fifo_rd),
  .wr_ack(),
  .data_count(cmd_fifo_data_count)
);

//SCHEDULER

scheduler sched(
  .clk(sys_clk),
  .rst(mecobo_reset),
  .current_time(global_clock),
  .cmd_fifo_dout(sched_fifo_data),
  .cmd_fifo_empty(ebi_fifo_empty),
  .cmd_fifo_valid(sched_fifo_valid),
  .cmd_fifo_rd_en(sched_fifo_rd),

  .cmd_bus_addr(cmd_bus_addr),
  .cmd_bus_data(cmd_bus_data_in),
  .cmd_bus_en(cmd_bus_en),
  .cmd_bus_wr(cmd_bus_wr)
);


// CONTROL MODULES
// -------------------------------------
//Standard pin controllers
genvar i;
generate
`ifdef WITH_DB
  for (i = 0; i < 16; i = i + 1) begin: pinControl 
    pincontrol #(.POSITION(i))
    pc (
      .clk(sys_clk),
      .reset(mecobo_reset),
      .enable(cmd_bus_en),
      .addr(cmd_bus_addr),
      .data_wr(cmd_bus_wr),
      .data_in(cmd_bus_data_in),
      .data_rd(),
      .data_out(),
      .pin(HW[i+1]),
      .output_sample(sample_enable_output),
      .channel_select(sample_channel_select),
      .sample_data(sample_data_bus),
      .current_time(global_clock)
    );
    //end
  end //for end

  adc_control #(
    .MIN_CHANNEL(100),   //note: these are addresses on the command bus for this unit as well. 
    .MAX_CHANNEL(107))
  adc0 (
    .clk(sys_clk),
    .sclk(ad_clk),
    .reset(mecobo_reset),
    .enable(cmd_bus_en),
    .re(),
    .wr(cmd_bus_wr),
    .addr(cmd_bus_addr),
    .data_in(cmd_bus_data_in),
    .data_out(),
    //interface to the world
    .cs(HN[48]),
    .adc_din(HN[32]),
    .adc_dout(HN[40]),
    .output_sample(sample_enable_output),
    .channel_select(sample_channel_select),
    .sample_data(sample_data_bus),
    .current_time(global_clock)
  );

  dac_control #(.POSITION(240))
  dac0 (
    .ebi_clk(sys_clk),
    .sclk(da_clk),
    .reset(mecobo_reset),
    .cmd_bus_addr(cmd_bus_addr),
    .cmd_bus_enable(cmd_bus_en),
    .re(),
    .cmd_bus_wr(cmd_bus_wr),
    .cmd_bus_data(cmd_bus_data_in),
    .out_data(),
    .nLdac(HN[24]),
    .nSync(HN[16]),
  .dac_din(HN[8]));

  xbar_control #(.POSITION(241))
  xbar0 (
    .ebi_clk(sys_clk),
    .sclk(xbar_clk),
    .reset(mecobo_reset),
    .cmd_bus_enable(cmd_bus_en),
    .re(),
    .cmd_bus_wr(cmd_bus_wr),
    .data_out(),
    .cmd_bus_data(cmd_bus_data_in),
    .cmd_bus_addr(cmd_bus_addr),
    .xbar_clock(HN[6]), //clock from xbar and out to device 
    .pclk(HN[1]),
  .sin(HN[9]));

`else
  for (i = 0; i < 10; i = i + 1) begin: pinControl 
    pincontrol #(.POSITION(i))
    pc (
      .clk(sys_clk),
      .reset(mecobo_reset),
      .enable(cmd_bus_en),
      .addr(cmd_bus_addr),
      .data_wr(cmd_bus_wr),
      .data_in(cmd_bus_data_in),
      .data_rd(),
      .data_out(),
      .pin(HN[i+1]),
      .output_sample(sample_enable_output),
      .channel_select(sample_channel_select),
      .sample_data(sample_data_bus),
      .current_time(global_clock)
    );



    //end
  end //for end
`endif
endgenerate

sample_collector sample_collector0(
  .clk(sys_clk),
  .rst(mecobo_reset),
  .addr(cmd_bus_addr),
  .cmd_data_in(cmd_bus_data_in),
  .cs(cmd_bus_en),
  .wr(cmd_bus_wr),
  .sample_data(sample_data_bus),
  .output_sample(sample_enable_output),
  .channel_select(sample_channel_select),

  .sample_fifo_rd_en(sample_collector_rd_en),
  .sample_data_out(sample_collector_data),
  .sample_fifo_empty(sample_fifo_empty),
  .sample_fifo_almost_empty(sample_fifo_almost_empty),
  .sample_fifo_full(sample_fifo_full),
  .sample_fifo_almost_full(sample_fifo_almost_full),
  .sample_fifo_data_count(sample_fifo_data_count)
);


// ------------------------- SAMPLING ---------------------
// Sampling module will access the ADC register at a given sampling rate.
// ADC runs at 16MHz, giving 1MHz max sample rate. We have 64Mbit of
// sample space, 12 bit samples, so at 1MHz we have about 4 seconds of samples,
// with some wasted space.
// The ADC will raise an interrupt when it has a new sample available, and 
// the sampling module will fetch this and put it into memory.
//
endmodule

