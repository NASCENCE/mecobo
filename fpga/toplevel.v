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
input [20:0] ebi_addr;
input ebi_wr;
input ebi_rd;
input ebi_cs;

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

assign led = 1'b1;
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
    pincontrol #(.POSITION(i*32))
    pc (
      .clk(clk),
      .reset(reset),
      .addr(ebi_ram_addr),
      .data_in(ebi_ram_data_out),
      .data_out(ebi_ram_data_in),
      .pin(pin[i])
    );
end
endgenerate

//Instaciate 100 pin controllers, hook them directly onto the EBI
//interface. Eat drink and be merry.
//
endmodule
