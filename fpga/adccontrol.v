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

  input [18:0] addr,
  input [15:0] data_in,
  input enable,
  input re,
  input wr,
  output reg [15:0] data_out,

  // interfacing the chip.
  output reg cs,
  //data line into the cip.
  output adc_din,
  //serial data from chip
  input adc_dout
);
                                            
//                          |offset                | channel| command
//Address layout: [18,17,16,15,14,13,12 | 11,10,9,8| 7,6,5,4,3,2,1,0]
parameter POSITION = 0;

wire controller_enable;
assign controller_enable = (enable & (addr[18:8] == POSITION));

wire busy = (state != get_values);

localparam [3:0] 
  PROGRAM  = 4'h4,
  OVERFLOW = 4'h1,
  DIVIDE   = 4'h2,
  SAMPLE   = 4'h7,
  SEQUENCE = 4'h8,
  ID_REG   = 4'h9,
  BUSY     = 4'hA,
  LAST     = 4'hB;

//Data capture from EBI
//--------------------------------------------------------------------
reg[15:0] ebi_capture_reg = 0;
always @ (posedge clk) begin
  if (reset) 
    data_out <= 0;
  else begin

//    if (reset_ebi_capture_reg) begin
 //     ebi_capture_reg <= 0;
  //  end

    if (controller_enable & wr) begin
      //Decode the command field.
      if (addr[3:0] == PROGRAM) begin
        ebi_capture_reg <= data_in;
      end
      //Data driving
      if (addr[3:0] == OVERFLOW) begin
        overflow_register[addr[7:4]] <= data_in;
      end

      if (addr[3:0] == DIVIDE) begin
        clock_divide_register <= data_in;
      end
    end

    //Driving out
    if(controller_enable & re) begin
      //Getting sample values.
      if (addr[3:0] == SAMPLE) begin
        data_out <= sample_register[addr[7:4]];
      end

      if (addr[3:0] == SEQUENCE) begin
        data_out <= sequence_number[addr[7:4]];
      end


      //Getting sample values.
      if (addr[3:0] == ID_REG) begin
        data_out <= 16'h0ADC;
      end

      //Check if busy.
      if (addr[3:0] == BUSY) begin
          data_out <= {15'h0, busy};
      end

      if (addr[3:0] == LAST) begin
          data_out <= last_executed_command;
      end

      end else
        data_out <= 0;
    end
  end
  //---------------------------------------------------------------------


reg [15:0] clock_divide_register = 0;
reg [15:0] overflow_register [0:7];
reg [15:0] tmp_register [0:7];   //Holds stored values ready to be harvested by the 'fast' timed state machine
reg [15:0] sample_register [0:7];   //Holds stored values ready to be harvested by the 'fast' timed state machine
reg [15:0] sequence_number [0:7]; //Holds sequence number.
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

reg load_current_command;
reg reset_current_command;

reg load_last_executed_command;
 


//----------------------------- SLOW DATA PATH -----------------------------------------------
//This describes the process that shifts out data in our out of the ADC.
//

reg [15:0] current_command = 0;
reg [15:0] last_executed_command = 0;

always @ (posedge sclk)  begin
  if (!reset) begin

    if (load_last_executed_command) begin
      last_executed_command <= current_command;
    end

    if (reset_current_command) begin
      current_command <= 0;
    end else if (load_current_command) begin
      current_command <= ebi_capture_reg;
    end

    //Clock counter logic
    if (res_clkcounter) 
      clkcounter <= 0; 
    else begin
      if (inc_clkcounter) begin
        clkcounter <= (clkcounter + 1);
      end
    end

    //shift reg logic. 
    //the fast clock loads the ebi capture reg, the slow clock loads the real shift register in sync
    //with the control state machine.
    if (load_shift_out_reg) 
      shift_out_register <= current_command;
    else if (shift_out)
      shift_out_register <= {shift_out_register[14:0], 1'b0};
    else
      shift_out_register <= 16'h0000;  //keep adc din tied low.
    end
    //Copy from shift register to temp register every 16 cycles. (first bits indicate which channel it's from)
    if (copy_to_tmp) begin
      $display("Copy from shift %h", shift_in_register[15:13]);
      tmp_register[shift_in_register[15:13]] <= shift_in_register;
    end
end

//Handle clocking in of data.
always @ (negedge sclk) begin
  if (shift_in) begin
    shift_in_register  <= {shift_in_register[14:0], adc_dout};
  end
end


//Take top-most bit of shift_out_register and hook it directly to ADC input.
assign adc_din = shift_out_register[15];
//----------------------------------------------------------------------------
parameter NUM_STATES    = 5;
parameter init          = 5'b00001;
parameter program_adc   = 5'b00010;
parameter get_values    = 5'b00100;
parameter copy          = 5'b01000;
parameter writeback     = 5'b10000;
//parameter reset_ebi     = 5'b10000;

reg [NUM_STATES - 1:0] state;

//---------------------------- CONTROL LOGIC -----------------------------
//The slow clocked control logic that controls the shift registers
//and times when to copy data into the temporary registers.
//TODO: Split into two state machines? One for shifting in, one for shifting out.


//Each state machine is two processes to make things a little more readable.
//The first process describes the next-state equation,
//and the second process describes the outputs for each state (it's combinatorial).

/*
always @ (posedge sclk) 
begin
  if (reset) begin
    state <= init;
  end
  else begin
    case(state)

      init: begin
        //If write bit is high, there's a new command in town.
        if (current_command[15] == 1'b1) begin
          //$display("Going to programming state, shifting out %d", shift_out_register);
          state <= program_adc;
        end else begin
          state <= get_values;  //ebi_capture 0, go get values by default.
        end
      end
      
      copy: begin
        state <= init; //via init state after copy to check for new commands.
      end

      get_values: begin
        state <= get_values;
        
        if (clkcounter == 15) begin
          state <= copy;
        end
      end

      //In programming state, for 16 sckl-cycles we will output data.
      program_adc: begin
        state <= program_adc;
        if (clkcounter == 15) begin
          state <= writeback;
        end
      end

      writeback: begin
        state <= init;
      end

      default:
        state <= init;

    endcase
  end
end
*/
//output equations and stunff.
always @ (posedge sclk) begin
  if (reset) begin
    cs <= 1'b1;
    shift_in <= 1'b0;  //shifts data into ADC
    shift_out <= 1'b0; //shifts data out from ADC
    load_shift_out_reg <= 1'b0;
    inc_clkcounter <= 1'b0;
    res_clkcounter <= 1'b0;
    copy_to_tmp <= 1'b0;

    load_current_command <= 1'b0;
    reset_current_command <= 1'b0;

    load_last_executed_command <= 1'b0;
    state <= init;

  end 
  else begin
    case (state)
      init: begin
        cs <= 1'b1;
        load_shift_out_reg <= 1'b0;
        shift_out <= 1'b0;
        shift_in <= 1'b0;
        inc_clkcounter <= 1'b0;
        res_clkcounter <= 1'b1;
        copy_to_tmp <= 1'b0;

        load_current_command <= 1'b1;
        reset_current_command <= 1'b0;

        load_last_executed_command <= 1'b0;

        if (current_command != last_executed_command) begin
          load_shift_out_reg <= 1'b1;
          state <= program_adc;
        end
        else begin
          state <= get_values;
        end


      end

    //This state shall copy to tmp registers.
    copy: begin
      cs <= 1'b1;   
      shift_in <= 1'b0;  //shifts data in from ADC
      shift_out <= 1'b0; //shifts data out to ADC
      load_shift_out_reg <= 1'b0;  //Shift out has to be 0 during fetching.
      inc_clkcounter <= 1'b0;
      res_clkcounter <= 1'b1;
      copy_to_tmp <= 1'b0;

      load_current_command <= 1'b1;
      reset_current_command <= 1'b0;

      load_last_executed_command <= 1'b0;
      state <= init;
    end


    get_values: begin
      cs <= 1'b0;  //pull low to select ADC.
      shift_in <= 1'b1;  //shifts data from ADC
      shift_out <= 1'b0; 
      load_shift_out_reg <= 1'b0;  //Shift out has to be 0 during fetching.
      inc_clkcounter <= 1'b1;  //note : increase on next posedge.
      res_clkcounter <= 1'b0;
      copy_to_tmp <= 1'b0;

      load_current_command <= 1'b0;
      reset_current_command <= 1'b0;

      load_last_executed_command <= 1'b0;

      state <= get_values;
      if(clkcounter == 15) begin
        state <= copy;
        cs <= 1'b1;
        copy_to_tmp <= 1'b1;
        shift_in <= 1'b0;
      end

    end

    writeback: begin
      cs <= 1'b1;  //pull low to select ADC.
      shift_in <= 1'b0;  //shifts data from ADC
      shift_out <= 1'b0; 
      load_shift_out_reg <= 1'b0;  //Shift out has to be 0 during fetching.
      inc_clkcounter <= 1'b0; 
      res_clkcounter <= 1'b1;
      copy_to_tmp <= 1'b0;

      load_current_command <= 1'b0;
      reset_current_command <= 1'b1;   //should be safe, loaded it last cycle.

      load_last_executed_command <= 1'b0;
      state <= init;

    end

    program_adc: begin
      cs <= 1'b0;
      shift_in <= 1'b0;  //shifts data in to ADC
      shift_out <= 1'b1; //shifts data out to ADC
      load_shift_out_reg <= 1'b0;
      inc_clkcounter <= 1'b1;
      res_clkcounter <= 1'b0;
      copy_to_tmp <= 1'b0;

      load_current_command <= 1'b0;
      reset_current_command <= 1'b0;
      load_last_executed_command <= 1'b0; 
      state <= program_adc;

      if(clkcounter == 15) begin
        load_last_executed_command <= 1'b1; //synchronize here
        state <= writeback;
      end     
    end

    default: begin
      cs <= 1'b1;
      shift_in <= 1'b0;  //shifts data into ADC
      shift_out <= 1'b0; //shifts data out from ADC
      load_shift_out_reg <= 1'b0;
      inc_clkcounter <= 1'b0;
      res_clkcounter <= 1'b0;
      copy_to_tmp <= 1'b0;

      load_current_command <= 1'b0;
      reset_current_command <= 1'b0;

      load_last_executed_command <= 1'b0;
      state <= init;
    end
  endcase
end
end


//------------------ Handles copying on overflow to real data register -----------
//TODO: statemachine that controls the sample rate copying.
//Controlled by fast clock. 
genvar i;
for (i = 0; i < 8; i = i+1) begin : sample_rate_registers
  always @ (posedge clk) begin
    if (reset) begin
      sample_register[i] <= 0;
      sequence_number[i] <= 0;
    end else
      if (fast_clk_counter[i] == overflow_register[i]) begin
        fast_clk_counter[i] <= 0;
        sample_register[i] <= tmp_register[i];
        sequence_number[i] <= sequence_number[i] + 1;
      end else begin
        fast_clk_counter[i] <= (fast_clk_counter[i] + 1);
      end
  end
end

endmodule
