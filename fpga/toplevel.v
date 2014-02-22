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


module mecobo (clk, reset, led, ebi_data, ebi_addr, ebi_wr, ebi_rd, ebi_cs, fpga_ready, pin);

input clk;
input reset;

inout [15:0] ebi_data;
input [18:0] ebi_addr;
input ebi_wr;
input ebi_rd;
input ebi_cs;

//output [15:0] debug;

output led;
output fpga_ready;
inout [99:0] pin;

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
assign led = led_heartbeat[22] | led_heartbeat[23];

always @ (posedge clk) begin
  if(reset)
    led_heartbeat <= 0;
  else
    led_heartbeat[23:0] <= led_heartbeat + 1'b1;
end

genvar i;
generate
  for (i = 0; i < 75 ; i = i + 1) begin: pinControl 
    pincontrol #(.POSITION(i))
    pc (
      .clk(clk),
      .reset(reset),
      .enable(chip_select),
      .addr(ebi_addr),
      .data_wr(write_enable),
      .data_in(data_in),
      .data_rd(read_enable),
      .data_out(data_out),
      .pin(pin[i])
    );
  end

endgenerate

//Instaciate 100 pin controllers, hook them directly onto the EBI
//interface. Eat drink and be merry.
//
endmodule
