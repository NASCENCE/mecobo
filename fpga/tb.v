module pwm_tb;

reg clk,reset;
reg [255:0] sample;
wire q;

reg [15:0] ebi_data;
reg [20:0] ebi_addr;
reg ebi_wr;
reg ebi_rd;
reg ebi_cs;


initial begin
  clk = 0;
  reset = 1;
  sample = 1;
  ebi_rd = 1'b0;

  #5
  reset = 0;
  ebi_addr = 1;
  ebi_wr = 1'b1;
  ebi_cs = 1'b1;
  ebi_data = 16'h000F;
 
  //cs and wr has to be held for at least 1 FPGA cycle
  #11
  ebi_cs = 1'b0;
  ebi_wr = 1'b0;
  //hold address at least 2 FPGA cycles 
  #11
  ebi_addr = 0;

  //Wait some time
  #42
  ebi_cs = 1'b1;
  ebi_addr = 1'b1;
  ebi_rd = 1'b1;
  #11
  ebi_cs = 1'b0;
  ebi_rd = 1'b0;
  #11
  ebi_addr = 0;

end

//clockin'
always begin
  #5 clk = !clk;
end

wire [15:0] fjas;
assign fjas = ebi_data;

mecobo mecobo0 (
.clk(clk),
.reset(reset),
.ebi_data(fjas),
.ebi_addr(ebi_addr),
.ebi_wr(ebi_wr),
.ebi_rd(ebi_rd),
.ebi_cs(ebi_cs));

/*
pwm pwm0 (
  .clk(clk),
  .reset(reset),
  .data_in(sample),
  .pwm_out(q)
);
*/
endmodule
