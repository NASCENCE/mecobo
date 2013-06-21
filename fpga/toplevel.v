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


module mecobo (clk, reset, ebi_data, ebi_addr, ebi_wr, ebi_rd, ebi_cs);

input clk;
input reset;
inout [15:0] ebi_data;
input [20:0] ebi_addr;
input ebi_wr;
input ebi_rd;
input ebi_cs;

wire ebi_clk;


assign ebi_clk = clk;
// EBI module communicates with the uC.
ebi_interface ebi0 (
  .clk(ebi_clk),
  .reset(reset),
  .data(ebi_data),
  .addr(ebi_addr),
  .wr(ebi_wr),
  .rd(ebi_rd),
  .cs(ebi_cs)
);



endmodule
