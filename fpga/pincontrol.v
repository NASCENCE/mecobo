module pincontrol ( clk,
  reset,
  enable,
  addr,
  data_wr,
  data_rd,
  data_in,
  data_out,
  pin,
  output_sample,
  channel_select,
  sample_data,
  current_time);



input clk;
input reset;
input enable;
input [18:0] addr;
input   [31:0] data_in;
//(* tristate2logic = "yes" *)
output reg [15:0] data_out;
inout pin;

input data_wr;
input data_rd;

input [31:0] current_time;

input output_sample;
input [7:0] channel_select;
output reg [31:0] sample_data;


parameter POSITION = 0;

reg sample_register = 0;
reg [15:0] sample_cnt = 16'h0000;
reg [31:0] nco_counter = 32'h00000000;
reg [31:0] nco_pa = 0;
reg [31:0] end_time = 0;


wire pin_input;
wire enable_in = (enable & (addr[15:8] == POSITION));
/*Input, output: PWM, SGEN, CONST */



/*These are byte addresses. */
localparam
ADDR_GLOBAL_CMD = 0,
  ADDR_NCO_COUNTER = 1,
  ADDR_END_TIME = 2,
  ADDR_LOCAL_CMD =  3,
  ADDR_SAMPLE_RATE =  4,
  ADDR_SAMPLE_REG =  5,
  ADDR_SAMPLE_CNT =  7,
  ADDR_STATUS_REG =  8,
  ADDR_LAST_DATA = 9;


localparam
  CMD_CONST = 2,
  CMD_SQUARE_WAVE = 3,
  CMD_INPUT_STREAM = 4,
  CMD_RESET = 5;

reg [31:0] cmdbus_captured_data;
reg [31:0] command = 0;
reg [31:0] sample_rate = 0;
reg [31:0] cnt_sample_rate = 0;


/*outputs from state machine */
reg res_cmd_reg = 1'b0;
reg res_sample_counter = 0;
reg dec_sample_counter = 0;
reg update_data_out    = 0;
reg update_sample_cnt = 0;
reg enable_pin_output = 0;
reg const_output_null = 0;
reg const_output_one = 0;

localparam [3:0] 
                    idle =            4'b0001,
                    const =           4'b0010,
                    input_stream =    4'b0100,
                    enable_out =      4'b1000;

reg [4:0] state = idle;

/* Command bus data output */
always @ (posedge clk) begin
  if(reset) begin
    sample_data <= 32'hZ;
    data_out <= 0;
  end else begin

    if (enable_in & data_rd) begin
      if (addr[7:0] == ADDR_SAMPLE_REG) 
        data_out <= {15'b0, sample_register};
      else if (addr[7:0] == ADDR_SAMPLE_CNT) 
        data_out <= sample_cnt;
      else if (addr[7:0] == ADDR_STATUS_REG)
        data_out <= POSITION;
      else if (addr[7:0] == ADDR_LAST_DATA)
        data_out <= cmdbus_captured_data[15:0];
      else
        data_out <= 16'b0;
    end else
      data_out <= 16'b0;

    if (output_sample & (channel_select == POSITION)) 
      sample_data <= {sample_cnt, 12'hABC, 3'b111, sample_register};
    else 
      sample_data <= 32'hZ;

  end
end


/* Drive output pin from MSB of nco_pa statemachine if output is enabled. */
assign pin = (enable_pin_output) ? nco_pa[31] : 1'bZ; /*Z or 0 */
assign pin_input = pin;


/* ------------------ CAPTURE FROM COMMAND BUS ---------*/
always @ (posedge clk) begin
  if (reset)
    cmdbus_captured_data <= 0;
  else if (enable_in & data_wr) begin
    cmdbus_captured_data <= data_in;
  end
end

always @ (posedge clk) begin
  if (reset)
    nco_counter <= 0;
  else if (res_cmd_reg) begin
    command <= 0;
  end else begin
    if (enable_in & data_wr) begin
      if (addr[7:0] == ADDR_LOCAL_CMD)
        command <= data_in;
      else if (addr[7:0] == ADDR_SAMPLE_RATE)
        sample_rate <= data_in;
      else if (addr[7:0] == ADDR_NCO_COUNTER)
        nco_counter[31:0] <= data_in;
      else if (addr[7:0] == ADDR_END_TIME)
        end_time[31:0] <= data_in;
    end
  end
end


/* CONTROL LOGIC STATE MACHINE */

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

      const_output_null <= 1'b0;
      const_output_one <= 1'b0;

      state <= idle;
      /*Check command register for waiting command. */

      //We don't start doing stuff if the time has not started, but we still
        //have a command waiting of course.
      if (current_time == 0) begin
        state <= idle;
      end else if (command == CMD_INPUT_STREAM) begin
        state <= input_stream;
        res_cmd_reg <= 1'b1; /*reset command since this is a single command. */
      end else if (command == CMD_SQUARE_WAVE) begin
        state <= enable_out;
        res_cmd_reg <= 1'b1; 
      end else if (command == CMD_CONST) begin
        state <= const;
        res_cmd_reg <= 1'b1;
      end else if (command == CMD_RESET) begin
        state <= idle;
        res_cmd_reg <= 1'b1;
      end
    end

    enable_out: begin
      dec_sample_counter <= 1'b0;
      res_sample_counter <= 1'b0;

      update_data_out <= 1'b0;
      enable_pin_output <= 1'b1;
      res_cmd_reg <= 1'b0;

      const_output_null <= 1'b0;
      const_output_one <= 1'b0;

      state <= enable_out;

      if (command == CMD_RESET) begin
        res_cmd_reg <= 1'b1;
        state <= idle;
      end else if ((end_time != 0) | (current_time >= end_time)) begin
        res_cmd_reg <= 1'b1;
        state <= idle;
      end
    end

    const: begin
      dec_sample_counter <= 1'b0;
      res_sample_counter <= 1'b0;

      update_data_out <= 1'b0;
      enable_pin_output <= 1'b1;
      res_cmd_reg <= 1'b0;

      const_output_null <= 1'b0;
      const_output_one <= 1'b1;

      state <= const;

      if (command == CMD_RESET) begin
        res_cmd_reg <= 1'b1;
        state <= idle;
      end else if (current_time >= end_time) begin
        res_cmd_reg <= 1'b1;
        state <= idle;
      end
    end

    /*Stream back data. */
    input_stream: begin
      res_cmd_reg <= 1'b0;

      res_sample_counter <= 1'b0;
      update_data_out <= 1'b0;
      enable_pin_output <= 1'b0;
      dec_sample_counter <= 1'b0;

      const_output_null <= 1'b0;
      const_output_one <= 1'b0;

      if (cnt_sample_rate <= 1) begin
        update_data_out <= 1'b1; 
        res_sample_counter <= 1'b1;
      end else begin
        update_data_out <= 1'b0; 
        dec_sample_counter <= 1'b1;
      end

      /*We're streaming input back
      at a certain rate, and will never leave this state
      unless reset is called.
        */
       if (command == CMD_RESET) begin
         res_cmd_reg <= 1'b1;
         state <= idle;
       end else begin
         state <= input_stream;
       end
    end 

    default: begin
       res_cmd_reg <= 1'b0;
       dec_sample_counter <= 1'b0;
       res_sample_counter <= 1'b0;
       update_data_out <= 1'b0;
       enable_pin_output <= 1'b0;

       const_output_null <= 1'b0;
       const_output_one <= 1'b0;
    end
  endcase
end

/* STATE MACHINE ENDS */



/* COUNTER REGISTERS CONTROLLED BY STATE MACHINE */

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


 /* ------------------------- NCO ----------------------------------
 Output frequency will be roughly freq(clk) * (nco_counter/2^32)
 */
always @ (posedge clk) begin
  if(reset)
    nco_pa <= 0;
  else begin
    if (const_output_null)
      nco_pa <= 0;
    else if (const_output_one) 
      nco_pa <= 32'hFFFFFFFF;
    else
      nco_pa <= nco_pa + nco_counter;
  end
end

endmodule
