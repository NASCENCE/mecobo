// Two clock domains: //  CK1 The controller domain (runs at system clock)
//  CK2 The the ADC domain; which is the serial clock in/out to the chip.
// 
// CK1 is used for access to the EBI interface and is also used for the state 
// machine that updates access_reg every clock cycle.
// CK2 is used for for clocking the DOUT and DIN lines as well as the shift reg that eats DIN
// and DOUT.

module adc_control (
  input clk,   //main system clock (75Mhz?)
  input sclk,  //clocks the serial interface.
  input reset,

  input [15:0] addr,
  input [31:0] data_in,
  input enable,
  input re,
  input wr,
  output reg [15:0] data_out,

  input output_sample,
  input [7:0] channel_select,
  output reg [31:0] sample_data,

  // interfacing the chip.
  output reg cs,
  //data line into the cip.
  output adc_din,
  //serial data from chip
  input adc_dout,

  input [31:0] current_time
);



//---------- State machine control outputs 

reg [3:0] adc_clocktick_counter;
//Control lines from controlling state machine to data path
reg res_adc_clocktick_counter;
reg res_outreg;
reg inc_adc_clocktick_counter;
reg shift_out; //shift out to ADC
reg shift_in; //shift into FPGA
reg copy_to_tmp;
reg load_shift_out_reg;
reg reset_current_command;
reg reset_rec_time;

//---------------------------------------------

//                          |offset                | channel| command
//Address layout: [18,17,16,15,14,13,12 | 11,10,9,8| 7,6,5,4,3,2,1,0]
parameter MIN_CHANNEL = 0;
parameter MAX_CHANNEL = 1;

wor  controller_enable_ch;
wor  channels_selected;
genvar ch;
for (ch = MIN_CHANNEL; ch <= MAX_CHANNEL; ch = ch + 1) begin : enable_for_generator
  assign controller_enable_ch = (addr[15:8] == ch);
  assign channels_selected = (channel_select == ch);
end
//assign controller_enable = enable & ((MIN_CHANNEL <= addr[15:8]) & (addr[15:8] <= MAX_CHANNEL));
assign controller_enable = enable & controller_enable_ch;


wire [3:0] chan_idx = channel_select-MIN_CHANNEL;
wire [3:0] int_chan_idx = addr[15:8]-MIN_CHANNEL;

reg [31:0] current_command = 0;
reg [31:0] last_executed_command = 0;

//Assumes 8 ADC channels, which is why a lot of these have second indices
//These registers 
reg [31:0] overflow_register [0:7];
reg [15:0] sequence_number [0:7]; //Holds sequence number.
reg [31:0] fast_clk_counter[0:7];

reg [15:0] shift_out_register;   //Register that holds the data that is to be shifted into the ADC
reg [15:0] shift_in_register;    //Data from the ADC. Every 16th clock cycle this will be clocked into tmp_register.

reg [15:0] tmp_register [0:7];   //Holds stored values ready to be harvested by the 'fast' timed state machine
reg [15:0] sample_register [0:7];   //Holds the actual samples from each channel
reg [31:0] end_time [0:7];

reg [31:0] rec_start_time [0:7];

wire busy;

integer q;
initial begin
  for(q = 0; q < 8; q = q + 1) begin
    tmp_register[q] = 0;
    sample_register[q] = 0;
    sequence_number[q] = 0;
    fast_clk_counter[q] = 1;
  end
end




localparam [3:0] 
PROGRAM  = 4'h4,
OVERFLOW = 4'h1,
DIVIDE   = 4'h2,
ENDTIME  = 4'h3,
SAMPLE   = 4'h7,
SEQUENCE = 4'h8,
ID_REG   = 4'h9,
BUSY     = 4'hA,
LAST     = 4'hB,
REC_START_TIME = 4'hC;

localparam ADC_CMD_NEW_VALUE = 1;

//Data capture from EBI
//--------------------------------------------------------------------
integer c;
reg[31:0] ebi_capture_reg = 0;
always @ (posedge clk) begin
  if (reset)  begin
    data_out <= 0;
    sample_data <= 32'hE5E6E7E8;
    for (c = 0; c < 8; c = c+1) begin
      overflow_register[c] <= 0;
      end_time[c] <= 0;
      rec_start_time[c] <= 0;
    end
    current_command <= 0;
  end else begin
    if (output_sample & channels_selected) begin
      //sample_data <= {sequence_number[chan_idx], sample_register[chan_idx]};
      if ((rec_start_time[chan_idx] != 0) & (rec_start_time[chan_idx] < current_time) & (end_time[chan_idx] > current_time))
        sample_data <= {sequence_number[chan_idx], sample_register[chan_idx]};
      else
        sample_data <= 32'hDEADBEEF;

    end else begin
        sample_data <= 32'hE5E6E7E8;
    end


    //As per protocol, there will be at least one cycle after a new command 
    //comes in to give the controller time to enter the correct state AND 
    //reset the incomming command register.

    if(reset_current_command)
      current_command <= 0;
    else begin 
      if (controller_enable & wr) begin
        //Decode the command field.
        if (addr[3:0] == PROGRAM) begin
          current_command <= data_in;
        end
      end
    end

    //recording time is a special register that allows one to 
    //start recording at some predetermined time.
    //Since we depart from idle state using this register,
    //it needs similar logic as a command, which is another 
    //way of leaving idle.
    if (reset_rec_time) begin
      for (c = 0; c < 8; c = c+1) begin
	rec_start_time[c] <= 0;
      end
    end else begin
      if (controller_enable & wr) begin
        if (addr[3:0] == REC_START_TIME) begin
         rec_start_time[int_chan_idx] <= data_in;
        end
      end
    end
   
    //The other registers can be written to whenever. 
    if (controller_enable & wr) begin
        //Data driving
        if (addr[3:0] == OVERFLOW) begin
          overflow_register[int_chan_idx] <= data_in;
        end

                if (addr[3:0] == ENDTIME) begin
          end_time[int_chan_idx] <= data_in;
        end
    end

    //Driving output bus.
    if(controller_enable & re) begin
      //Getting sample values.
      if (addr[3:0] == SAMPLE) begin
        data_out <= sample_register[int_chan_idx];
      end

      if (addr[3:0] == SEQUENCE) begin
        data_out <= sequence_number[int_chan_idx];
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

  end //reset ifelse enmd
end //always end





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

reg [NUM_STATES - 1:0] state, nextState;


assign busy = (state != get_values);
//---------------------------- CONTROL LOGIC -----------------------------
//The slow clocked control logic that controls the shift registers
//and times when to copy data into the temporary registers.


always @ (posedge sclk) begin
  if (reset) state <= init; 
  else state <= nextState;
end


always @ (*) begin

  nextState = 5'bXXXXX;

  cs = 1'b1;
  shift_in = 1'b0;  //shifts data into ADC
  shift_out = 1'b0; //shifts data out from ADC
  load_shift_out_reg = 1'b0;
  inc_adc_clocktick_counter = 1'b0;
  res_adc_clocktick_counter = 1'b0;
  copy_to_tmp = 1'b0;

  reset_current_command = 1'b0;
  reset_rec_time = 1'b0;


  case (state)
    init: begin
      res_adc_clocktick_counter = 1'b1;
      nextState = init;

      if (current_command[31:16] == ADC_CMD_NEW_VALUE) begin
        load_shift_out_reg = 1'b1;
        nextState = program_adc;
      end else begin
        nextState = get_values;
      end
    end

    //This nextState shall copy to tmp registers.
    copy: begin
      res_adc_clocktick_counter = 1'b1;

      //we are done, so reset the rec time so that
      //no new samples are collected
      if (end_time[chan_idx] <= current_time)
         reset_rec_time = 1'b1;
     
      nextState = init;
    end

    get_values: begin
      cs = 1'b0;  //pull low to select ADC.
      shift_in = 1'b1;  //shifts data from ADC
      inc_adc_clocktick_counter = 1'b1;  
      nextState = get_values;
      if(adc_clocktick_counter == 15) begin
        nextState = copy;
        copy_to_tmp = 1'b1;  //16 ticks have been se..een... next flank, copy.
      end
    end

    program_adc: begin
      cs = 1'b0;  //remember, control signal is /not/ clocked
      shift_out = 1'b1; //shifts data out to ADC
      inc_adc_clocktick_counter = 1'b1;
      reset_current_command = 1'b1;
      nextState = program_adc;

      if(adc_clocktick_counter == 15) begin
      	res_adc_clocktick_counter = 1'b1;
        nextState = init;
      end     
    end
  endcase
end


//------------------ Handles copying on overflow to real data register -----------
//TODO: statemachine that controls the sample rate copying.
//Controlled by fast clock. 
//Since the slow clock is 10Hz, and we have 16 ticks before a new value is ready in tmp, 
//it takes at least 1600ns before a new value is ready-- or 1.6uS. 
//this means that it doesn't make sense for overflow_register to be less
//than 120. Oh well.
genvar i;
for (i = 0; i < 8; i = i+1) begin : sample_rate_registers
  always @ (posedge clk) begin
    if (reset) begin
      sample_register[i] <= 0;
      sequence_number[i] <= 0;
      fast_clk_counter[i] <= 1;
    end else
      if (fast_clk_counter[i] == overflow_register[i]) begin   //fast counter overflows, select sample rate.
        fast_clk_counter[i] <= 1;
        sample_register[i] <= tmp_register[i];
        sequence_number[i] <= sequence_number[i] + 1;
      end else begin
        fast_clk_counter[i] <= (fast_clk_counter[i] + 1);
      end
  end
end




//----------------------------- SLOW DATA PATH -----------------------------------------------
//This describes the process that shifts out data in our out of the ADC.

always @ (posedge sclk)  begin
  if (~reset) begin

    //ADC clock counter
    if (res_adc_clocktick_counter) 
      adc_clocktick_counter <= 0; 
    else begin
      if (inc_adc_clocktick_counter) begin
        adc_clocktick_counter <= (adc_clocktick_counter + 1);
      end
    end

    //shift reg logic. 
    //the fast clock loads the ebi capture reg, the slow clock loads the real shift register in sync
    //with the control state machine.
    if (load_shift_out_reg) 
      shift_out_register <= current_command[15:0];
    else if (shift_out)
      shift_out_register <= {shift_out_register[14:0], 1'b0};
    else
      shift_out_register <= 16'h0000;  //keep adc din tied low.
  end
  //Copy from shift register to temp register every 16 cycles. (first bits indicate which channel it's from)
  if (copy_to_tmp) begin
    //$display("Copy from shift %h", shift_in_register[15:13]);
    tmp_register[shift_in_register[15:13]] <= shift_in_register;
  end
end



//Handle clocking in of data, note negative clock edge.
always @ (negedge sclk) begin
  if (shift_in) begin
`ifdef DEBUG
      shift_in_register  <= {shift_in_register[14:0], 1'b1};
`else
    shift_in_register  <= {shift_in_register[14:0], adc_dout};
`endif
  end
end


endmodule
