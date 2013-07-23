		module pwm_tb;

reg clk,reset;
reg [255:0] sample;
wire q;

reg [15:0] ebi_data;
reg [20:0] ebi_addr;
reg ebi_wr;
reg ebi_rd;
reg ebi_cs;

reg [15:0] got;

initial begin
  clk = 0;
  reset = 1;
  sample = 1;
  ebi_rd = 1'b0;

//pin controller 0

  #21
  //Duty cycle write
  reset = 0;
  ebi_wr = 1;
  ebi_addr = 1;
  ebi_data = 2;
  #21
  //Anti duty write
  ebi_addr = 2;
  ebi_data = 2; //2 cycles low
  #21
  //Run for 5 cycles.	
  ebi_addr = 3;
  ebi_data = 5; //total 5 cycles run
  #21
  ebi_addr = 5; //local command address for pin 0
  ebi_data = 1; //command : start
  #21
  ebi_wr = 0; //stop doing stuff.
  ebi_data = 0;
  ebi_addr = 0;

  /*
  #21
  //Send local command to start being an input!
  ebi_addr = 16'h0106; //set sample rate register
  ebi_data = 10; //sample every 10 cycles
  #21
  ebi_addr = 16'h0108;
  ebi_data = 3; //set pin mode
  #21 
  ebi_addr[15:0] = 16'h0105; //set sample rate register
  ebi_data = 3; //start capture command

  #21
  //Global command reg 0, start output.
  ebi_addr = 0; 
  ebi_data = 1;

  #21
  ebi_wr = 0;
  ebi_rd = 1;
  ebi_addr[15:0] = 16'h0107; //set sample rate register
  got = ebi_data;
    $display ("got: %b\n", got);
  */

end

//clockin'
always begin
  #10 clk = !clk;
end

wire [15:0] fjas;
assign fjas = ebi_data;

wire [15:0] pins;
wire jmp;
assign jmp = pins[0];
assign pins[1] = jmp;
assign pins [15:2] = 14'b0;

mecobo mecobo0 (
.clk(clk),
.reset(reset),
.ebi_data(fjas),
.ebi_addr(ebi_addr),
.ebi_wr(ebi_wr),
.ebi_rd(ebi_rd),
.ebi_cs(ebi_cs),
.pin(pins));

/*
pwm pwm0 (
  .clk(clk),
  .reset(reset),
  .data_in(sample),
  .pwm_out(q)
);
*/
endmodule
