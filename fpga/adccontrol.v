// Two clock domains:
//  CK1 The controller domain (runs at system clock)
//  CK2 The the ADC domain; which is the serial clock in/out to the chip.
// 
// CK1 is used for access to the EBI interface and is also used for the state 
// machine that updates access_reg every clock cycle.
// CK2 is used for for clocking the DOUT and DIN lines as well as the shift reg that eats DIN
// and DOUT.



module adc_control (
  input clk,   //main system clock (100Mhz?)
  input sclk,  //clocks the serial interface.
  input reset,

  input [20:0] addr,
  input [15:0] data_in,
  input re,
  input wr,
  output reg [15:0] data_out,

  output reg busy,


  // interfacing the chip.
  output reg cs,
  //data line into the cip.
  output adc_din,
  //serial data from chip
  input adc_dout
);
                                            
//                       Channel          Command    Offset (position)
//Address layout: [18,17,16,15,14,13,12 | 11,10,9,8| 7,6,5,4,3,2,1,0]
parameter POSITION = 0;

reg controller_enable = 0;
always @ (posedge clk) begin
  if (addr[7:0] == POSITION)
    controller_enable = 1;
  else 
    controller_enable = 0;
end


localparam [3:0] 
  PROGRAM  = 4'h0,
  OVERFLOW = 4'h1,
  DIVIDE   = 4'h2,
  SAMPLE   = 4'h3;

//Data capture from EBI
//--------------------------------------------------------------------
reg[15:0] tmp_shift_out_reg;
always @ (posedge clk) begin
  if (reset) 
    tmp_shift_out_reg <= 0;
  else
  //"Chip select" decode
    if (controller_enable && wr) begin
      //Decode the command field.
      if (addr[11:8] == PROGRAM) begin
        //if not shifting out data we are free to capture new.
        if (!shift_out) begin
          tmp_shift_out_reg <= data_in;
        end 
      end
      
      if (addr[11:8] == OVERFLOW) begin
        overflow_register[addr[18:12]] <= data_in;
      end

      if (addr[11:8] == DIVIDE) begin
        clock_divide_register <= data_in;
      end

      //Getting sample values.
      if (addr[11:8] == SAMPLE && re) begin
        $display("Someone asked for sample for channel %d: %h\n", addr[18:12],sample_register[addr[18:12]]);
        data_out <= sample_register[addr[18:12]];
      end

    end else
      data_out <= 0;
end
//---------------------------------------------------------------------


reg [15:0] clock_divide_register = 0;
reg [15:0] overflow_register [0:7];
reg [15:0] tmp_register [0:7];   //Holds stored values ready to be harvested by the 'fast' timed state machine
reg [15:0] sample_register [0:7];   //Holds stored values ready to be harvested by the 'fast' timed state machine
reg [15:0] fast_clk_counter[0:7];
reg [15:0] shift_out_register;   //Register that holds the data that is to be shifted into the ADC
reg [15:0] shift_in_register;    //Data from the ADC. Every 16th clock cycle this will be clocked into tmp_register.


reg [3:0] clkcounter;
//Control lines from controlling state machine to data path
reg res_clkcounter;
reg res_outreg;
reg inc_clkcounter;
reg shift_out; //shift out to ADC
reg shift_in; //shift into FPGA
reg copy_to_tmp;
reg load_shift_out_reg;


//----------------------------- SLOW DATA PATH -----------------------------------------------
//This describes the process that shifts out data in our out of the ADC.
always @ (posedge sclk)  begin
  if (!reset) begin
    if (res_clkcounter) 
      clkcounter <= 0; 
    else begin
      if (inc_clkcounter) begin
        clkcounter <= (clkcounter + 1);
      end
    end

    //shift reg logic. 
    //the fast clock loads the temp reg, the slow clock loads the real shift register.
    if (load_shift_out_reg) 
      shift_out_register <= tmp_shift_out_reg;
    else if (shift_out) begin
      shift_out_register <= {shift_out_register[14:0], 1'b0};
    end else begin
      shift_out_register <= 16'h0000;  //keep adc din tied low.
    end

    if (shift_in) begin
      shift_in_register  <= {shift_in_register[14:0], adc_dout};
    end else begin
      shift_in_register <= 16'h0000; //keep it zero when not shifting in data.
    end

    //Copy from shift register to temp register every 16 cycles. (first bits indicate which channel it's from)
    if (copy_to_tmp) begin
      $display("Copy from shift %h", shift_in_register[15:13]);
      tmp_register[shift_in_register[15:13]] <= shift_in_register;
    end
  end
end

//Take top-most bit of shift_out_register and hook it directly to ADC input.
assign adc_din = shift_out_register[15];
//----------------------------------------------------------------------------
parameter NUM_STATES = 3;
parameter init         = 2'b00;
parameter program_adc   = 2'b01;
parameter get_values   = 2'b10;

reg [NUM_STATES - 1:0] state;

//---------------------------- CONTROL LOGIC -----------------------------
//The slow clocked control logic that controls the shift registers
//and times when to copy data into the temporary registers.
always @ (posedge sclk) 
begin
  if (reset) begin
    cs <= 1'b1;
    state <= init;
    shift_in <= 1'b0;
    shift_out <= 1'b0;
  end
  else begin
    case(state)

      init: begin
        busy <= 1'b0;
        cs <= 1'b1;
        load_shift_out_reg <= 1'b1;
        shift_out <= 1'b0;
        shift_in <= 1'b0;
        inc_clkcounter <= 1'b0;
        res_clkcounter <= 1'b1;
        copy_to_tmp <= 1'b0;
        state <= init;

        //If write bit is high, there's a new command in town.
        if (shift_out_register[15] == 1'b1) begin
          $display("Going to programming state, shifting out %d", shift_out_register);
          state <= program_adc;
          //shift_out <= 1'b1;
          //cs <= 1'b0;
        end else begin
          //Go get values if no commands to program. 
          state <= get_values;
          //cs <= 1'b0;
          //shift_in <= 1'b0;
        end
      end
      
      get_values: begin
        busy <= 1'b1;
        cs <= 1'b0;
        shift_in <= 1'b1;  //shifts data into ADC
        shift_out <= 1'b0; //shifts data out from ADC
        load_shift_out_reg <= 1'b0;
        inc_clkcounter <= 1'b1;
        res_clkcounter <= 1'b0;
        copy_to_tmp <= 1'b0;

        //Keep getting values if we're not done.
        state <= get_values;

        //done with getting one sample, let's go via init
        //to make sure we're synchronized.
        if (clkcounter == 15) begin
          //Next cycle we should copy.
          copy_to_tmp <= 1'b1;
        //  res_clkcounter <= 1'b1;
         // inc_clkcounter <= 1'b0;
          state <= init;  //go back to initial state to check for new commands. 
          //cs <= 1'b1;
        end
      end

      //In programming state, for 16 sckl-cycles we will output data.
      program_adc: begin
        busy <= 1'b1;
        cs <= 1'b0;
        shift_in <= 1'b0;  //shifts data into ADC
        shift_out <= 1'b1; //shifts data out from ADC
        load_shift_out_reg <= 1'b0;
        inc_clkcounter <= 1'b1;
        res_clkcounter <= 1'b0;
        copy_to_tmp <= 1'b0;

        //Keep getting values if we're not done.
        state <= program_adc;
        //done with getting one sample, let's go via init
        //to make sure we're synchronized.
        if (clkcounter == 15) begin
          //Next cycle we should copy.
          //res_clkcounter <= 1'b1;
          //inc_clkcounter <= 1'b0;
          //cs <= 1'b1; //raise chip select again. no more data.
          state <= init;
        end
      end

    endcase
  end
end

//------------------ Handles copying on overflow to real data register -----------
//TODO: statemachine that controls the sample rate copying.
//Controlled by fast clock. 
genvar i;
for (i = 0; i < 8; i = i+1) begin
  always @ (posedge clk) begin
      if (fast_clk_counter[i] == overflow_register[i]) begin
        fast_clk_counter[i] <= 0;
        sample_register[i] <= tmp_register[i];
      end else begin
        fast_clk_counter[i] <= (fast_clk_counter[i] + 1);
      end
  end
end

endmodule
