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

//pin controller 0

  #21
  reset = 0;
  ebi_wr = 1;
  ebi_addr = 4;
  ebi_data = 2;
  #21
  ebi_wr = 1;
  ebi_addr = 8;
  ebi_data = 2;
  #21
  ebi_addr = 12;
  ebi_data = 5;


  
  //pin controller 1

  #21
  ebi_wr = 1;
  ebi_addr = 32 + 4;
  ebi_data = 4;
  #21
  ebi_wr = 1;
  ebi_addr = 32 + 8;
  ebi_data = 4;
  #21
  ebi_addr = 32 + 12;
  ebi_data = 5;

  #21
  ebi_addr = 0;
  ebi_data = 1;
  #21
  ebi_data = 0;
  ebi_wr = 0;

end

//clockin'
always begin
  #10 clk = !clk;
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
