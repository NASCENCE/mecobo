module pincontrol (clk, reset, addr, data, data_rd, data_wr, pin);

input clk;
input reset;
input [20:0] addr;
inout [15:0] data;
wire [15:0] data_in;
//output [15:0] data_out;
inout pin;

input data_wr;
input data_rd;

reg [15:0] sample_register = 16'b0;

reg pin_output;
wire pin_input;
//Input, output: PWM, SGEN, CONST
localparam [3:0] 
  MODE_UNCONFIGURED   = 4'b0000,
  MODE_OUTPUT = 4'b0001,
  MODE_INPUT  = 4'b0010,
  MODE_INPUT_STREAM = 4'b0011;



wire enable = ((addr == ADDR_GLOBAL_CMD) || (addr[15:8] == POSITION));
//Drive output pin from pin_output statemachine if mode is output
assign pin = (enable_pin_output) ? pin_output : 1'bZ;
//else we have input from pin.
assign pin_input = pin;


//Position is the offset in the address map to this pin controller.
parameter POSITION = 0;
localparam BASE_ADDR = (POSITION << 8);

localparam [20:0] 
  ADDR_GLOBAL_CMD = 0, //Address 0 will be a global command register.
  ADDR_DUTY_CYCLE = BASE_ADDR + 1,
  ADDR_ANTI_DUTY_CYCLE = BASE_ADDR + 2,
  ADDR_CYCLES = BASE_ADDR + 3,
  ADDR_RUN_INF = BASE_ADDR + 4,
  ADDR_LOCAL_CMD = BASE_ADDR + 5,
  ADDR_SAMPLE_RATE = BASE_ADDR + 6,
  ADDR_SAMPLE_REG = BASE_ADDR + 7,
  ADDR_PIN_MODE = BASE_ADDR + 8;

always @ (posedge clk) begin
  if (res_cmd_reg)
    command <= 0;
  else if (enable & data_wr) begin
    if ((addr == ADDR_GLOBAL_CMD) || (addr == ADDR_LOCAL_CMD))
      command <= data_in;
    else if (addr == ADDR_DUTY_CYCLE)
      duty_cycle <= data_in;
    else if (addr == ADDR_ANTI_DUTY_CYCLE)
      anti_duty_cycle <= data_in;
    else if (addr == ADDR_CYCLES)
      cycles <= data_in;
    else if (addr == ADDR_RUN_INF)
      run_inf <= data_in;
    else if (addr == ADDR_SAMPLE_RATE)
      sample_rate <= data_in;
    else if (addr == ADDR_PIN_MODE)
      pin_mode <= data_in;
  end 
end

//Command parse
localparam
  CMD_START_OUTPUT = 1,
  LOCAL_CMD_WRITE_PIN = 2,
  CMD_INPUT_STREAM = 3,
  LOCAL_CMD_START_OUTPUT = 4,
  CMD_RESET = 5;

reg [15:0] command = 0;
//reg [15:0] command = 0;

reg [15:0] duty_cycle = 0; //length of duty in cyle, measured in 20ns ticks.
reg [15:0] anti_duty_cycle = 0; //length of anti-duty in 20ns ticks. 
reg [15:0] cycles = 0; //number of cycles to run
reg [15:0] run_inf = 0; //set to 1 if we just want to run inf.
reg [15:0] sample_rate = 0;

//Counters for the cycles.
reg [15:0] cnt_duty_cycle = 0;
reg [15:0] cnt_anti_duty_cycle = 0;
reg [15:0] cnt_cycles = 0;
reg [15:0] cnt_sample_rate = 0;
reg [15:0] pin_mode = 0;

always @ (posedge clk) begin
  if (reset) begin
    cnt_duty_cycle <= 0;
    cnt_anti_duty_cycle <= 0;
    cnt_cycles <= 0;
  end

  if (res_duty_counter == 1'b1)
    cnt_duty_cycle <= duty_cycle;
  else if (dec_duty_counter == 1'b1) 
    cnt_duty_cycle <= cnt_duty_cycle - 16'h0001;

  if (res_anti_duty_counter == 1'b1)
    cnt_anti_duty_cycle <= anti_duty_cycle;
  else if (dec_anti_duty_counter == 1'b1) 
    cnt_anti_duty_cycle <= cnt_anti_duty_cycle - 4'b0001;

  if (run_inf == 0) begin
    if (res_cycles_counter == 1'b1)
      cnt_cycles <= cycles;
    else if (dec_cycles_counter == 1'b1) 
      cnt_cycles <= cnt_cycles - 16'h0001;
  end

  if (res_sample_counter == 1'b1) 
    cnt_sample_rate <= sample_rate;
  else if (dec_sample_counter == 1'b1) 
    cnt_sample_rate <= cnt_sample_rate - 16'h0001;

  if (update_data_out) 
    sample_register <= { 15'b0, pin_input };

end

//Drive output, or tristate against bus.
assign data = (enable & data_rd) ? sample_register : 16'bZ;
assign data_in = data;
	
//outputs from state machine
reg dec_duty_counter;
reg dec_anti_duty_counter;
reg dec_cycles_counter;
reg res_duty_counter;
reg res_anti_duty_counter;
reg res_cycles_counter;

reg res_cmd_reg = 1'b0;

reg res_sample_counter = 0;
reg dec_sample_counter = 0;
reg update_data_out    = 0;

reg enable_pin_output = 0;

reg [3:0] state;
reg [3:0] next_state;

localparam [3:0] 
  idle = 4'b0001,
  high = 4'b0010,
  low  = 4'b0100,
  input_stream = 4'b1000;

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
      dec_cycles_counter <= 1'b0;

      res_duty_counter <= 1'b1;
      res_anti_duty_counter <= 1'b1;
      res_cycles_counter <= 1'b1;

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
        next_state <= high;
        res_cmd_reg <= 1'b1; //reset command since this is a single command.
      end else
        next_state <= idle;

      pin_output <= 1'b0;
    end

  high: begin
    dec_duty_counter <= 1'b1;
    res_duty_counter <= 1'b0; 

    dec_anti_duty_counter <= 1'b0;
    dec_cycles_counter <= 1'b0;

    res_anti_duty_counter <= 1'b0;
    res_cycles_counter <= 1'b0;

    dec_sample_counter <= 1'b0;
    res_sample_counter <= 1'b0;

    update_data_out <= 1'b0;
    enable_pin_output <= 1'b1;
    pin_output <= 1'b1;
    res_cmd_reg <= 1'b0;

    if (cnt_duty_cycle == 1) begin
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
    
    dec_cycles_counter <= 1'b0;
    res_cycles_counter <= 1'b0;

    dec_sample_counter <= 1'b0;
    res_sample_counter <= 1'b0;

    update_data_out <= 1'b0;
    res_cmd_reg <= 1'b0;
    
    enable_pin_output <= 1'b1;
    pin_output <= 1'b0;

    if (command == CMD_RESET) 
      next_state <= idle;
    else if (cnt_anti_duty_cycle == 1) begin
      //last low-cycle, reset the anti duty counter
      //so that it's ready for next time.
      res_anti_duty_counter <= 1'b1;
      dec_cycles_counter <= 1'b1;
      //Special case if we're running inf for this.
      if (run_inf) 
        next_state <= high;
      else if (cnt_cycles == 1) begin
        //last full cycle, we're done with
        //this command and should return to idle.
        next_state <= idle;
      end else begin
        next_state <= high;
      end
    end 
  end

  input_stream: begin
    res_cmd_reg <= 1'b0;
    dec_duty_counter <= 1'b0;
    dec_anti_duty_counter <= 1'b0;
    dec_cycles_counter <= 1'b0;

    res_duty_counter <= 1'b1;
    res_anti_duty_counter <= 1'b1;
    res_cycles_counter <= 1'b1;

    res_sample_counter <= 1'b0;
    update_data_out <= 1'b0;
    enable_pin_output <= 1'b0;
    pin_output <= 1'b0;

    //If we have counted down to 1, it's time to update sample reg.
    if (cnt_sample_rate == 1) begin
      update_data_out <= 1'b1; 
      dec_sample_counter <= 1'b0;
    end else begin
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
        dec_cycles_counter <= 1'b0;

        res_duty_counter <= 1'b1;
        res_anti_duty_counter <= 1'b1;
        res_cycles_counter <= 1'b1;

        dec_sample_counter <= 1'b0;
        res_sample_counter <= 1'b0;

        update_data_out <= 1'b0;

        enable_pin_output <= 1'b0;
        pin_output <= 1'b0;
      end
    endcase
end

endmodule
