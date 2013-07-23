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


module mecobo (clk, reset, led, ebi_data, ebi_addr, ebi_wr, ebi_rd, ebi_cs, fpga_ready, pin, debug);

input clk;
input reset;
inout [15:0] ebi_data;
input [20:0] ebi_addr;
input ebi_wr;
input ebi_rd;
input ebi_cs;

output [7:0] debug;

assign debug[0] = ebi_wr;
assign debug[1] = ebi_addr[0];
assign debug[2] = ebi_addr[8];
assign debug[3] = ebi_data[0];
assign debug[4] = ebi_data[1];
assign debug[5] = ebi_data[2];
assign debug[6] = ebi_data[3];
assign debug[7] = ebi_data[4];
output led;
output fpga_ready;
inout [15:0] pin;

assign fpga_ready = 1'b0;

wire [15:0] ebi_ram_data_in;
wire [15:0] ebi_ram_data_out;
wire [20:0] ebi_ram_addr;
wire ebi_ram_wr;
wire ebi_ram_en;


wire [15:0] cmd_ram_data_in;
wire [15:0] cmd_ram_data_out;
wire [20:0] cmd_ram_addr;
wire cmd_ram_wr;
wire cmd_ram_en;

reg [23:0] led_heartbeat = 0;
assign led = led_heartbeat[22] | led_heartbeat[23];

always @ (posedge clk) begin
  led_heartbeat <= led_heartbeat + 1;
end

//EBI goes straight into Block RAM, just via a thin EBI layer with tristates on data.
/*
dp_ram shmem (
  .clka(clk),
  .clkb(clk),
  .ena(ebi_ram_en),
  .enb(cmd_ram_en),
  .wea(ebi_ram_wr),
  .web(cmd_ram_wr),
  .addra(ebi_ram_addr),
  .addrb(cmd_ram_addr),
  .dia(ebi_ram_data_out),
  .dib(cmd_ram_data_out),
  .doa(ebi_ram_data_in),
  .dob(cmd_ram_data_in)
);
*/
/*
ebi_interface ebi0 (
  .clk(clk),
  .reset(reset),
  .ebi_data(ebi_data),
  .ebi_addr(ebi_addr),
  .ebi_wr(ebi_wr),
  .ebi_rd(ebi_rd),
  .ebi_cs(ebi_cs),
  .ram_data_in(ebi_ram_data_in),
  .ram_data_out(ebi_ram_data_out),
  .ram_addr(ebi_ram_addr),
  .ram_wr(ebi_ram_wr),
  .ram_en(ebi_ram_en)
);
*/
/*
mecoCommand cmd (
  .clk(clk),
  .reset(reset),
  .ram_data_in(cmd_ram_data_in),
  .ram_data_out(cmd_ram_data_out),
  .ram_addr(cmd_ram_addr),
  .ram_wr(cmd_ram_wr),
  .ram_en(cmd_ram_en),
  .pin_out(pin_out)
);
*/

genvar i;
generate
  for (i = 0; i < 16 ; i = i + 1) begin: pinControl 
    pincontrol #(.POSITION(i))
    pc (
      .clk(clk),
      .reset(reset),
      .addr(ebi_addr),
      .data(ebi_data),
      .data_wr(ebi_wr),
 //     .data_in(ebi_ram_data_out),
      .data_rd(ebi_rd),
 //     .data_out(ebi_ram_data_in),
      .pin(pin[i])
    );
end
endgenerate

//Instaciate 100 pin controllers, hook them directly onto the EBI
//interface. Eat drink and be merry.
//
endmodule
