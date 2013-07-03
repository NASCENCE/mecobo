module pincontrol (clk, reset, addr, data_in, data_out, pin);

input clk;
input reset;
input [20:0] addr;
input [15:0] data_in;
output reg [15:0] data_out;
inout reg pin;

reg pin_output;
reg pin_input;
//Input, output: PWM, SGEN, CONST
localparam [1:0] 
  MODE_OUTPUT = 2'b00,
  MODE_INPUT  = 2'b01;

reg [1:0] mode = 2'b00;

//Drive output pin from pin_output statemachine if mode is output
assign pin = (mode == MODE_OUTPUT) ? pin_output : 1'bZ;
//else we have input from pin.
assign pin_input = (mode == MODE_INPUT) ? pin;

parameter POSITION = 0;

localparam [20:0] 
ADDR_GLOBAL_CMD = 0, //Address 0 will be a global command register.
ADDR_DUTY_CYCLE = POSITION + 1,
ADDR_ANTI_DUTY_CYCLE = POSITION + 2,
ADDR_CYCLES = POSITION + 3,
ADDR_RUN_INF = POSITION + 4;
ADDR_LOCAL_COMMAND = POSITION + 5;
ADDR_SAMPLE_RATE = POSITION + 6;

always @ (posedge clk) begin
  if (addr == ADDR_GLOBAL_CMD)
    global_command <= data_in;
  else if (addr == ADDR_DUTY_CYCLE)
    duty_cycle <= data_in;
  else if (addr == ADDR_ANTI_DUTY_CYCLE)
    anti_duty_cycle <= data_in;
  else if (addr == ADDR_CYCLES)
    cycles <= data_in;
  else if (addr == ADDR_RUN_INF)
    run_inf <= data_in;  
  else if (addr == ADDR_LOCAL_COMMAND)
   local_command <= data_in;
  else if (addr == ADDR_SAMPLE_RATE)
    sample_rate <= data_in;
end


//local command doing schmooing. 
localparam 
  LOCAL_CMD_READ_PIN = 1,
  LOCAL_CMD_WRITE_PIN = 2,
  LOCAL_CMD_START_CAPTURE = 3,
  LOCAL_CMD_START_OUTPUT = 4;

reg enable_data_out = 1'b0;

always @ (posedge clk) begin
  if (local_command == LOCAL_CMD_START_CAPTURE)
    mode <= MODE_INPUT;
  else if (local_command == LOCAL_CMD_START_OUTPUT) begin
    mode <= MODE_OUTPUT;
  else if (local_command == LOCAL_CMD_READ_PIN) 
    enable_data_out <= 1'b1;
  end
end



//Properties of the signal generator
reg [15:0] global_command = 0;
reg [15:0] local_command = 0;
reg [15:0] duty_cycle = 0; //length of duty in cyle, measured in 20ns ticks.
reg [15:0] anti_duty_cycle = 0; //length of anti-duty in 20ns ticks. 
reg [15:0] cycles = 0; //number of cycles to run
reg [15:0] run_inf = 0; //set to 1 if we just want to run inf.
reg [15:0] sample_rate = 0;
//Captured sample if this is a input module.
reg [15:0] sample = 0;

//Counters for the cycles.
reg [15:0] cnt_duty_cycle;
reg [15:0] cnt_anti_duty_cycle;
reg [15:0] cnt_cycles;
reg [15:0] cnt_sample_rate;

always @ (posedge clk) begin
  if (reset) begin
    cnt_duty_cycle <= 0;
    cnt_anti_duty_cycle <= 0;
    cnt_cycles <= 0;
  end

  if (dec_duty_counter == 1'b1)
    cnt_duty_cycle <= cnt_duty_cycle - 16'b1;
  else if (res_duty_counter == 1'b1) 
    cnt_duty_cycle <= duty_cycle;

  if (dec_anti_duty_counter == 1'b1)
    cnt_anti_duty_cycle <= cnt_anti_duty_cycle - 16'b1;
  else if (res_anti_duty_counter == 1'b1) 
    cnt_anti_duty_cycle <= anti_duty_cycle;

  if (run_inf == 0) begin
    if (dec_cycles_counter == 1'b1)
      cnt_cycles <= cnt_cycles - 16'b1;
    else if (res_cycles_counter == 1'b1) 
      cnt_cycles <= cycles;
  end

  if (dec_sample_counter == 1'b1) 
    cnt_sample_counter == cnt_sample_counter - 16'b1;
  else if (res_sample_counter == 1'b1) 
    cnt_sample_rate <= sample_rate;
end



//outputs from state machine
reg dec_duty_counter;
reg dec_anti_duty_counter;
reg dec_cycles_counter;
reg res_duty_counter;
reg res_anti_duty_counter;
reg res_cycles_counter;

reg [2:0] state;
reg [2:0] next_state;

localparam [2:0] 
idle = 3'b001,
  high = 3'b010,
  low  = 3'b100;

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
      dec_duty_counter <= 1'b0;
      dec_anti_duty_counter <= 1'b0;
      dec_cycles_counter <= 1'b0;

      res_duty_counter <= 1'b1;
      res_anti_duty_counter <= 1'b1;
      res_cycles_counter <= 1'b1;

      pin_output <= 1'b0;

      if ((global_command == 15'b1) && (cnt_cycles != 0))
        next_state <= high;
    end

    high: begin
      dec_duty_counter <= 1'b1;
      res_duty_counter <= 1'b0; 

      dec_anti_duty_counter <= 1'b0;
      dec_cycles_counter <= 1'b0;

      res_anti_duty_counter <= 1'b0;
      res_cycles_counter <= 1'b0;
      pin_output <= 1'b1;

      if (cnt_duty_cycle == 1) begin
        next_state <= low;
        res_duty_counter <= 1'b1; 
      end
    end

    low: begin
      dec_duty_counter <= 1'b0;
      res_duty_counter <= 1'b0;
      dec_anti_duty_counter <= 1'b1;

      dec_cycles_counter <= 1'b1;
      res_cycles_counter <= 1'b0;

      if (cnt_anti_duty_cycle == 1) begin
        res_anti_duty_counter <= 1'b1;
        if (cnt_cycles == 1) begin
          next_state <= idle;
        end else begin
          next_state <= high;
        end
      end else begin
        dec_anti_duty_counter <= 1'b1;
        res_anti_duty_counter <= 1'b0;
      end

      pin_output <= 1'b0;
    end
    default: begin
      dec_duty_counter <= 1'b0;
      dec_anti_duty_counter <= 1'b0;
      dec_cycles_counter <= 1'b0;

      res_duty_counter <= 1'b1;
      res_anti_duty_counter <= 1'b1;
      res_cycles_counter <= 1'b1;
      pin_output <= 1'b0;
    end
  endcase
end

//Input state machine
assign data_out[0] = enable_data_out ? pin_input : 1'bZ;
reg [1:0] in_state;
reg [1:0] in_next_state;

//Outputs from state machine
reg res_sample_counter = 0;
reg dec_sample_counter = 0;
reg update_data_out    = 0;

localparam [1:0]
  in_idle: 2'b01,
  in_read_pin: 2'b10;

always @ (posedge clk) begin
  if (reset) 
    in_state <= idle;
  else 
    in_state <= in_next_state;
  end
end

always @ ( * ) begin
  in_state <= in_next_state;
  case (in_state)
    idle: begin
      //If mode is input
      in_next_state <= idle;
      dec_sample_counter <= 1'b0;
      res_sample_counter <= 1'b0;
      update_data_out <= 1'b0;

      if (mode == MODE_INPUT) begin
        if (cnt_sample_rate == 0) begin
          in_next_state <= in_read_pin;
          res_sample_counter <= 1'b1;
        end else begin
          dec_sample_counter <= 1'b1;
        end
      end
    end
    read_pin: begin
      dec_sample_counter <= 1'b0;
      res_sample_counter <= 1'b0;
      update_data_out <= 1'b1;
      in_next_state <= in_idle;
    end
    default: begin
      dec_sample_counter <= 1'b0;
      res_sample_counter <= 1'b0;
      update_data_out <= 1'b0;
    end
  endcase
end
endmodule
