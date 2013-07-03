module pincontrol (clk, reset, addr, data_in, data_out, pin_output);

input clk;
input reset;
input [20:0] addr;
input [15:0] data_in;
output [15:0] data_out;
output reg pin_output;

//Input, output: PWM, SGEN, CONST
reg [1:0] mode;

//Tie upper half of data to 0.
//assign data_in [15:8] = 8'b0;
assign data_out[15:0] = 8'b0;

parameter POSITION = 0;

localparam [20:0] 
ADDR_GLOBAL_CMD = 0, //Address 0 will be a global command register.
ADDR_DUTY_CYCLE = POSITION + 1,
ADDR_ANTI_DUTY_CYCLE = POSITION + 2,
ADDR_CYCLES = POSITION + 3,
ADDR_RUN_INF = POSITION + 4;
ADDR_LOCAL_COMMAND = POSITION + 5;

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
end


//Properties of the signal generator
reg [15:0] global_command = 0;
reg [15:0] local_command = 0;
reg [15:0] duty_cycle = 0; //length of duty in cyle, measured in 20ns ticks.
reg [15:0] anti_duty_cycle = 0; //length of anti-duty in 20ns ticks. 
reg [15:0] cycles = 0; //number of cycles to run
reg [15:0] run_inf = 0; //set to 1 if we just want to run inf.
//Captured sample if this is a input module.
reg [15:0] sample = 0;

//Counters for the cycles.
reg [15:0] cnt_duty_cycle;
reg [15:0] cnt_anti_duty_cycle;
reg [15:0] cnt_cycles;
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

endmodule
