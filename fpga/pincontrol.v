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
reg [31:0] nco_counter = 32'h00000000;
reg [31:0] nco_pa = 0;

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



// ------------------------- NCO ----------------------------------
// Output frequency will be roughly freq(clk) * (nco_counter/2^32)
always @ (posedge clk) begin
  nco_pa <= nco_pa + nco_counter;
end

//Drive output pin from MSB of nco_pa statemachine if output is enabled.
assign pin = (enable_pin_output) ? nco_pa[31] : 1'bZ; //Z or 0
//else we have input from pin.
assign pin_input = pin;


//Position is the offset in the address map to this pin controller.
parameter POSITION = 0;

//These are byte addresses.
localparam [7:0] 
  ADDR_GLOBAL_CMD = 0, //Address 0 will be a global command register.
  ADDR_NCO_COUNTER_LOW = 2,
  ADDR_NCO_COUNTER_HIGH = 3,
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
    else if (addr[7:0] == ADDR_SAMPLE_RATE)
      sample_rate <= data_in;
    else if (addr[7:0] == ADDR_NCO_COUNTER_LOW)
      nco_counter[15:0] <= data_in;
    else if (addr[7:0] == ADDR_NCO_COUNTER_HIGH)
      nco_counter[31:16] <= data_in;
  end 
end

//Command parse
localparam
  CMD_START_OUTPUT = 1,
  CMD_INPUT_STREAM = 3,
  CMD_RESET = 5;

reg [15:0] command = 0;
//reg [15:0] command = 0;

reg [15:0] sample_rate = 0;

//Counters for the cycles.
reg [15:0] cnt_sample_rate = 0;

always @ (posedge clk) begin

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
        state <= enable_out;
        res_cmd_reg <= 1'b1; //reset command since this is a single command
      //No command..
      end else
        state <= idle;
    end

  enable_out: begin
    dec_sample_counter <= 1'b0;
    res_sample_counter <= 1'b0;

    update_data_out <= 1'b0;
    enable_pin_output <= 1'b1;
    res_cmd_reg <= 1'b0;

    if (command == CMD_RESET) 
      state <= idle;
    end else
      state <= enable_out;
  end

  //Stream back data.
  input_stream: begin
    res_cmd_reg <= 1'b0;

    res_sample_counter <= 1'b0;
    update_data_out <= 1'b0;
    enable_pin_output <= 1'b0;
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
    dec_sample_counter <= 1'b0;
    res_sample_counter <= 1'b0;
    update_data_out <= 1'b0;
    enable_pin_output <= 1'b0;
  end
  endcase
end

endmodule
