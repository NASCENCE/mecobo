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
//`define WITH_OPTOWALL

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
//assign led[3] = chip_select;

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

`ifdef WITH_DB
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

// Stupid mistake. These fixes need to be
// applied to the physical daughterboard as well.
`ifdef WITH_DB
wire [15:0] pin_rerouting;
assign HW[1] = pin_rerouting[0];
assign HW[3] = pin_rerouting[1];  
assign HW[5] = pin_rerouting[2];
assign HW[7] = pin_rerouting[3];   
assign HW[2] = pin_rerouting[4];
assign HW[4] = pin_rerouting[5];
assign HW[13] = pin_rerouting[6]; 
assign HW[15] = pin_rerouting[7]; 
assign HW[6] = pin_rerouting[8];
assign HW[8] = pin_rerouting[9];  
assign HW[10] = pin_rerouting[10];
assign HW[14] = pin_rerouting[11];
assign HW[25] = pin_rerouting[12];
assign HW[27] = pin_rerouting[13];
assign HW[16] = pin_rerouting[14]; 
assign HW[31] = pin_rerouting[15];
`endif

`ifdef WITH_OPTOWALL
// Map Mecobo pins to optowall pins
wire [15:0] optowall_in_map;
wire [15:0] optowall_out_map_h;
wire [15:0] optowall_out_map_l;

// o3
assign optowall_in_map[0] = HN[17];
assign HN[8]  = optowall_out_map_h[0];
assign HN[6]  = optowall_out_map_l[0];

assign optowall_in_map[1] = HN[19];
assign HN[4]  = optowall_out_map_h[1];
assign HN[2]  = optowall_out_map_l[1];

assign optowall_in_map[2] = HN[21];
assign HN[1]  = optowall_out_map_h[2];
assign HN[3]  = optowall_out_map_l[2];

assign optowall_in_map[3] = HN[23];
assign HN[5]  = optowall_out_map_h[3];
assign HN[7]  = optowall_out_map_l[3];

// o2
assign optowall_in_map[4] = HN[9];
assign HN[10] = optowall_out_map_h[4];
assign HN[12] = optowall_out_map_l[4];

assign optowall_in_map[5] = HN[11];
assign HN[14] = optowall_out_map_h[5];
assign HN[16] = optowall_out_map_l[5];

assign optowall_in_map[6] = HN[13];
assign HN[18] = optowall_out_map_h[6];
assign HN[20] = optowall_out_map_l[6];

assign optowall_in_map[7] = HN[15];
assign HN[22] = optowall_out_map_h[7];
assign HN[24] = optowall_out_map_l[7];

// o1
assign optowall_in_map[8] = HN[25];
assign HN[33] = optowall_out_map_h[8];
assign HN[35] = optowall_out_map_l[8];

assign optowall_in_map[9] = HN[27];
assign HN[37] = optowall_out_map_h[9];
assign HN[39] = optowall_out_map_l[9];

assign optowall_in_map[10] = HN[30];
assign HN[41] = optowall_out_map_h[10];
assign HN[43] = optowall_out_map_l[10];

assign optowall_in_map[11] = HN[31];
assign HN[45] = optowall_out_map_h[11];
assign HN[47] = optowall_out_map_l[11];

// o0
assign optowall_in_map[12] = HN[48];
assign HN[32] = optowall_out_map_h[12];
assign HN[34] = optowall_out_map_l[12];

assign optowall_in_map[13] = HN[50];
assign HN[36] = optowall_out_map_h[13];
assign HN[38] = optowall_out_map_l[13];

assign optowall_in_map[14] = HN[52];
assign HN[40] = optowall_out_map_h[14];
assign HN[42] = optowall_out_map_l[14];

assign optowall_in_map[15] = HN[54];
assign HN[44] = optowall_out_map_h[15];
assign HN[46] = optowall_out_map_l[15];
`endif


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
        .reset(reset),
        .enable(chip_select),
        .addr(ebi_addr),
        .data_wr(write_enable),
        .data_in(data_in),
        .data_rd(read_enable),
        .data_out(data_out),
        .pin(pin_rerouting[i])
      );
    //end
  end //for end
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
		
  `elsif WITH_OPTOWALL
      for (i = 0; i < 16; i = i + 1) begin: pinControl 
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
			  .pin_in(optowall_in_map[i]),
			  .pins_out({optowall_out_map_h[i], optowall_out_map_l[i]})
			);
		end //for end
		
  `else
    for (i = 0; i < 54; i = i + 1) begin: pinControl 
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
        .pin(HN[i+1])
      );
      //end
    end //for end
  `endif

    endgenerate
endmodule

