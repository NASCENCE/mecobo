module pincontrol (clk, reset, enable, addr, data_wr, data_rd, data_in, data_out, pin);

input clk;
input reset;
input enable;
input [18:0] addr;
input   [15:0] data_in;
//(* tristate2logic = "yes" *)
output reg [15:0] data_out;
inout pin;

input data_wr;
input data_rd;

reg sample_register = 0;
reg [15:0] sample_cnt = 16'h0000;

reg pin_output;
wire pin_input;
//Input, output: PWM, SGEN, CONST
localparam [3:0] 
  MODE_UNCONFIGURED   = 4'b0000,
  MODE_OUTPUT = 4'b0001,
  MODE_INPUT  = 4'b0010,
  MODE_INPUT_STREAM = 4'b0011;


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

//Drive output pin from pin_output statemachine if mode is output
assign pin = (enable_pin_output) ? pin_output : 1'bZ;
//else we have input from pin.
assign pin_input = pin;


//Position is the offset in the address map to this pin controller.
parameter POSITION = 0;
localparam BASE_ADDR = (POSITION << 8);

//These are byte addresses.
localparam [20:0] 
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
//reg [15:0] pin_mode = 0;

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

reg [4:0] state;
reg [4:0] next_state;

localparam [4:0] 
  idle =          5'b0001,
  high =          5'b0010,
  low  =          5'b0100,
  input_stream =  5'b1000;

always @ (posedge clk) begin
  if (reset) 
    state <= idle;
  else 
    state <= next_state;
end

always @ ( * ) begin
  next_state <= state;
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
        next_state <= input_stream;
        res_cmd_reg <= 1'b1; //reset command since this is a single command.
      end
      //Output command.
      else if ( (command == CMD_START_OUTPUT) ) begin
        
        if(duty_cycle > 0)
          next_state <= high;
        else 
          next_state <= low;

        res_cmd_reg <= 1'b1; //reset command since this is a single command
      
      //No command..
      end else
        next_state <= idle;

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
      next_state <= idle;
    else if (cnt_duty_cycle <= 1) begin
      //check if we should just hang around here.
      if(anti_duty_cycle == 0)
        next_state <= high;
      else 
        next_state <= low;
      //Reset duty counter so that it's
      //ready for the next time we're in this state.
      res_duty_counter <= 1'b1; 
    end else
      next_state <= high;
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
      next_state <= idle;
    else if (cnt_anti_duty_cycle <= 1) begin
      //If we don't have any high cycles, go here.
      if(cnt_duty_cycle == 0) 
        next_state <= low;
      else begin
        //last low-cycle, reset the anti duty counter
        //so that it's ready for next time.
        res_anti_duty_counter <= 1'b1;
      
        next_state <= high;
      end
    end else 
      next_state <= low;
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
        next_state <= idle;
      else
        next_state <= input_stream;
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
