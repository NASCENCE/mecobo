//`define WITH_OPTOWALL

`ifdef WITH_OPTOWALL
  module pincontrol (clk, reset, enable, addr, data_wr, data_rd, data_in, data_out, pin_in, pins_out);
`else
  module pincontrol (clk, reset, enable, addr, data_wr, data_rd, data_in, data_out, pin);
`endif

input clk;
input reset;
input enable;
input [18:0] addr;
input   [15:0] data_in;
//(* tristate2logic = "yes" *)
output reg [15:0] data_out;
`ifdef WITH_OPTOWALL
  input pin_in;
  output[1:0] pins_out;
`else
  inout pin;
`endif

input data_wr;
input data_rd;

reg sample_register = 0;
reg [15:0] sample_cnt = 16'h0000;

reg pin_output;
wire pin_input;
//Input, output: PWM, SGEN, CONST

wire enable_in = (enable & (addr[18:8] == POSITION));

always @ (posedge clk) begin
  if (enable_in & data_rd) begin
    if (addr[7:0] == ADDR_SAMPLE_REG) 
      data_out <= {15'b0, sample_register};
    else if (addr[7:0] == ADDR_SAMPLE_CNT) 
      data_out <= sample_cnt;
    else if (addr[7:0] == ADDR_STATUS_REG)
      data_out <= POSITION;
    else
      data_out <= 16'b0;
  end else
    data_out <= 16'b0;
end

`ifdef WITH_OPTOWALL
  //Drive output pins to 01 (high), 10 (low) or 00 (Z)
  assign pins_out  = (enable_pin_output)? ((pin_output)?  2'b01 : 2'b10): 2'b00;
  assign pin_input = ~pin_in;
`else
  //Drive output pin from pin_output statemachine if output
  assign pin = (enable_pin_output) ? pin_output : 1'bZ; //Z or 0
  //else we have input from pin.
  assign pin_input = pin;
`endif


//Position is the offset in the address map to this pin controller.
parameter POSITION = 0;

//These are byte addresses.
localparam [7:0] 
  ADDR_GLOBAL_CMD = 0, //Address 0 will be a global command register.
  ADDR_DUTY_CYCLE =  1,
  ADDR_ANTI_DUTY_CYCLE =  2,
  ADDR_LOCAL_CMD =  5,
  ADDR_SAMPLE_RATE =  6,
  ADDR_SAMPLE_REG =  7,
  ADDR_SAMPLE_CNT =  8,
  ADDR_STATUS_REG =  9;

always @ (posedge clk) begin
  if (res_cmd_reg)
    command <= 0;
  else if (enable_in & data_wr) begin
    if (addr[7:0] == ADDR_LOCAL_CMD)
      command <= data_in;
    else if (addr[7:0] == ADDR_DUTY_CYCLE)
      duty_cycle <= data_in;
    else if (addr[7:0] == ADDR_ANTI_DUTY_CYCLE)
      anti_duty_cycle <= data_in;
    else if (addr[7:0] == ADDR_SAMPLE_RATE)
      sample_rate <= data_in;

  end 
end

//Command parse
localparam
  CMD_START_OUTPUT = 1,
  CMD_INPUT_STREAM = 3,
  CMD_RESET = 5;

reg [15:0] command = 0;
//reg [15:0] command = 0;

reg [15:0] duty_cycle = 0; //length of duty in cyle, measured in 20ns ticks.
reg [15:0] anti_duty_cycle = 0; //length of anti-duty in 20ns ticks. 
reg [15:0] sample_rate = 0;

//Counters for the cycles.
reg [15:0] cnt_duty_cycle = 0;
reg [15:0] cnt_anti_duty_cycle = 0;
reg [15:0] cnt_sample_rate = 0;

always @ (posedge clk) begin

  if (res_duty_counter == 1'b1)
    cnt_duty_cycle <= duty_cycle;
  else if (dec_duty_counter == 1'b1) 
    cnt_duty_cycle <= (cnt_duty_cycle - 16'h0001);

  if (res_anti_duty_counter == 1'b1)
    cnt_anti_duty_cycle <= anti_duty_cycle;
  else if (dec_anti_duty_counter == 1'b1) 
    cnt_anti_duty_cycle <= (cnt_anti_duty_cycle - 16'h0001);

  if (res_sample_counter == 1'b1) 
    cnt_sample_rate <= sample_rate;
  else if (dec_sample_counter == 1'b1) 
    cnt_sample_rate <= (cnt_sample_rate - 1);

  if (update_data_out)  begin
    sample_register <= pin_input;
    sample_cnt <= (sample_cnt + 1);
  end

end
//outputs from state machine
reg dec_duty_counter;
reg dec_anti_duty_counter;
reg res_duty_counter;
reg res_anti_duty_counter;

reg res_cmd_reg = 1'b0;

reg res_sample_counter = 0;
reg dec_sample_counter = 0;
reg update_data_out    = 0;

reg update_sample_cnt = 0;

reg enable_pin_output = 0;

reg [3:0] state = idle;

localparam [3:0] 
  idle =          4'b0001,
  high =          4'b0010,
  low  =          4'b0100,
  input_stream =  4'b1000;

always @ (posedge clk) begin
  if (reset)
    state <= idle;

  case (state)
    idle: begin
      enable_pin_output <= 1'b0;

      dec_duty_counter <= 1'b0;
      dec_anti_duty_counter <= 1'b0;

      res_duty_counter <= 1'b1;
      res_anti_duty_counter <= 1'b1;

      dec_sample_counter <= 1'b0;
      res_sample_counter <= 1'b1;

      res_cmd_reg <= 1'b0;
      update_data_out <= 1'b0;

      //Check command register for waiting command.
      if ( (command == CMD_INPUT_STREAM) ) begin
        state <= input_stream;
        res_cmd_reg <= 1'b1; //reset command since this is a single command.
      end
      //Output command.
      else if ( (command == CMD_START_OUTPUT) ) begin
        
        if(duty_cycle > 0)
          state <= high;
        else 
          state <= low;

        res_cmd_reg <= 1'b1; //reset command since this is a single command
      
      //No command..
      end else
        state <= idle;

      //keep it low
      pin_output <= 1'b0;
    end

  high: begin
    dec_duty_counter <= 1'b1;
    res_duty_counter <= 1'b0; 

    dec_anti_duty_counter <= 1'b0;
    res_anti_duty_counter <= 1'b0;

    dec_sample_counter <= 1'b0;
    res_sample_counter <= 1'b0;

    update_data_out <= 1'b0;
    enable_pin_output <= 1'b1;
    pin_output <= 1'b1;
    res_cmd_reg <= 1'b0;

    if (command == CMD_RESET) 
      state <= idle;
    else if (cnt_duty_cycle <= 1) begin
      //check if we should just hang around here.
      if(anti_duty_cycle == 0)
        state <= high;
      else 
        state <= low;
      //Reset duty counter so that it's
      //ready for the next time we're in this state.
      res_duty_counter <= 1'b1; 
    end else
      state <= high;
  end

  low: begin
    dec_duty_counter <= 1'b0;
    res_duty_counter <= 1'b0;
    
    dec_anti_duty_counter <= 1'b1;
    res_anti_duty_counter <= 1'b0;
    

    dec_sample_counter <= 1'b0;
    res_sample_counter <= 1'b0;

    update_data_out <= 1'b0;
    res_cmd_reg <= 1'b0;
    
    enable_pin_output <= 1'b1;
    pin_output <= 1'b0;

    if (command == CMD_RESET) 
      state <= idle;
    else if (cnt_anti_duty_cycle <= 1) begin
      //If we don't have any high cycles, go here.
      if(cnt_duty_cycle == 0) 
        state <= low;
      else begin
        //last low-cycle, reset the anti duty counter
        //so that it's ready for next time.
        res_anti_duty_counter <= 1'b1;
      
        state <= high;
      end
    end else 
      state <= low;
  end

  input_stream: begin
    res_cmd_reg <= 1'b0;
    dec_duty_counter <= 1'b0;
    dec_anti_duty_counter <= 1'b0;

    res_duty_counter <= 1'b1;
    res_anti_duty_counter <= 1'b1;

    res_sample_counter <= 1'b0;
    update_data_out <= 1'b0;
    enable_pin_output <= 1'b0;
    pin_output <= 1'b0;
    dec_sample_counter <= 1'b0;

    //If we have counted down to 1, it's time to update sample reg.
      if (cnt_sample_rate <= 1) begin
        update_data_out <= 1'b1; 
        res_sample_counter <= 1'b1;
      end else begin
        update_data_out <= 1'b0; 
        dec_sample_counter <= 1'b1;
      end

      //We're streaming input back
      //at a certain rate, and will never leave this state
      //unless reset is called.
      if (command == CMD_RESET)
        state <= idle;
      else
        state <= input_stream;
  end 

  default: begin
    res_cmd_reg <= 1'b0;
    dec_duty_counter <= 1'b0;
    dec_anti_duty_counter <= 1'b0;

    res_duty_counter <= 1'b1;
        res_anti_duty_counter <= 1'b1;

        dec_sample_counter <= 1'b0;
        res_sample_counter <= 1'b0;

        update_data_out <= 1'b0;

        enable_pin_output <= 1'b0;
        pin_output <= 1'b0;
      end
    endcase
end

endmodule
