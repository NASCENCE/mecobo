module adc_control (
  input clk;
  input reset;

  //Select channel to sense.
  input[3:0] channel;
  //return from the controller
  output [11:0] sample;

  // interfacing the chip.
  output cs;
  //data line into the cip.
  output adc_din;
  //clocking the data in line to the chip. Max 10Mhz on this clock. 
  output adc_sclk;
  //serial data from chip
  input adc_dout;

);

//1 load output register with a value.  //16 bits, this includes the output register ya.
//2 output register
//state machine

//a certain state loads the shift-register with a certain value and gets it clocked out. but we can just do it in
//the same state:
//
//
//
// init_1: write range regs for all channels: maybe just make a function that clocks out 
// bits ? wow, verilog functions. getting advanved are we.
task shift_register;
  input clk, si, load;
  input [15:0] value;
  output so;

  reg [15:0] tmp;
  always @ (posedge c)
  begin
    if (load)
      tmp <= value;
    else
      tmp <= {tmp[14:0], si};
  end

  assign so = tmp[15];
endtask
