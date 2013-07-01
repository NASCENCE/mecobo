module pincontrol (clk, reset, addr, data_in, data_out, done);

input clk;
input reset;
input [20:0] addr;
input [15:0] data_in;

//Input, output: PWM, SGEN, CONST
reg [1:0] mode;

parameter POSITION = 0;

localparam [20:0] 
  ADDR_GLOBAL_CMD = 0; //Address 0 will be a global command register.
  ADDR_DUTY_CYCLE = POSITION + 4;
  ADDR_ANTI_DUTY_CYCLE = POSITION + 8;

always @ (posedge clk) begin
  if (addr = ADDR_GLOBAL_CMD)
    global_command <= data_in;
  if (addr = ADDR_DUTY_CYCLE)
    duty_cycle <= data_in;
  if (addr= ADDR_ANTI_DUTY_CYCLE)
    anti_duty_cycle <= data_in;
end

//Check global command for interesting stuff. 
reg running = 1'b0;
always @ (posedge clk) begin
  if (global_command = 15'b1;) begin
    running <= 1'b1;
  end
end

//Properties of the signal generator
reg [15:0] global_command;
reg [15:0] local_command;
reg [15:0] duty_cycle; //length of duty in cyle, measured in 20ns ticks.
reg [15:0] anti_duty_cycle; //length of anti-duty in 20ns ticks. 
reg [15:0] cycles; //number of cycles to run
reg [15:0] run_inf; //set to 1 if we just want to run inf.
//Captured sample if this is a input module.
reg [15:0] sample;

//Counters for the cycles.
reg [15:0] cnt_duty_cycle;
reg [15:0] cnt_anti_duty_cycle;
reg [15:0] cnt_cycles;
always @ (posedge clk) 
  if (reset) begin
    cnt_duty_cycle <= 0;
    cnt_anti_duty_cycle <= 0;
    cnt_cycles <= 0;
  end

  if (dec_duty_counter = 1'b1)
    cnt_duty_cycle <= cnt_duty_cycle - 1;
  else if (res_duty_counter = 1'b1) 
    cnt_duty_cycle <= duty_cycle;

  if (dec_anti_duty_counter = 1'b1)
    cnt_anti_duty_cycle <= cnt_anti_duty_cycle - 1;
  else if (res_duty_counter = 1'b1) 
    cnt_anti_duty_cycle <= anti_duty_cycle;

  if (dec_cycles_counter = 1'b1)
    cnt_cycles <= cnt_cycles - 1;
  else if (res_duty_counter = 1'b1) 
    cnt_cycles <= cycles;
end



//outputs from state machine
reg dec_duty_counter;
reg dec_anti_duty_counter;
reg dec_cycles_counter;
reg res_duty_counter;
reg res_anti_duty_counter;
reg res_cycles_counter;
reg pin_output;

reg [2:0] state;
reg [2:0] next_state;

localparam [2:0] 
  idle = 3'b001;
  high = 3'b010;
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

      if (running)
        next_state <= high;
    end

    high: begin
      if (cnt_duty_cycle = 0) begin
        next_state <= low;
        dec_duty_counter <= 1'b0; 
      end else
        dec_duty_counter <= 1'b1;
     
      dec_anti_duty_counter <= 1'b0;
      dec_cycles_counter <= 1'b0;

      res_duty_counter <= 1'b0;
      res_anti_duty_counter <= 1'b0;
      res_cycles_counter <= 1'b0;
      

    end

    low: begin
      if (cnt_anti_duty_cycle = 0) begin
        if (cnt_cycles = 0) begin
          next_state <= idle;
          dec_anti_duty_counter <= 1'b1;
          dec_cycles_counter <= 1'b0;

          res_anti_duty_counter <= 1'b0;
          res_cycles_counter <= 1'b1;
        end else begin
          next_state <= high;

          dec_anti_duty_counter <= 1'b0;
          dec_cycles_counter <= 1'b1;

          res_anti_duty_counter <= 1'b1;
          res_cycles_counter <= 1'b0;

        end
      end else begin
        dec_anti_duty_counter <= 1'b1;
        dec_cycles_counter <= 1'b0;

        res_anti_duty_counter <= 1'b0;
        res_cycles_counter <= 1'b0;
      end

      dec_duty_counter <= 1'b0;
      res_duty_counter <= 1'b0;
    end

  endcase
end

endmodule
