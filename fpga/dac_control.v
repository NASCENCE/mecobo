module dac_control (
  input ebi_clk, //System clock
  input sclk, //Max 30MHz
  input reset,
  input cmd_bus_enable,
  input re,
  input cmd_bus_wr,
  input [31:0]cmd_bus_data,
  output reg [15:0] out_data,
  input [15:0] cmd_bus_addr,
  //facing the DAC
  output reg nLdac,
  output reg nSync,
output dac_din);

parameter POSITION = 240;
localparam DAC_CMD_NEW_VALUE = 1;

//Controller select (not to be confused with chip select)
wire cs;
assign cs = (cmd_bus_enable & (cmd_bus_addr[15:8] == POSITION));
wire busy;

reg [15:0] shift_out_register = 0;
reg [31:0] command_bus_data = 0;

reg reset_command_bus_data;

always @ (posedge ebi_clk) begin
  if (reset) begin
    out_data <= 16'h0000;
    command_bus_data <= 0;
  end
  else begin
    if (reset_command_bus_data) begin
      command_bus_data <= 0;
    end else
      if (cs & cmd_bus_wr)
        command_bus_data <=cmd_bus_data;

      //Driving outcmd_bus_data 
      if (cs & re) begin
        if (cmd_bus_addr[7:0] == 9) begin
          out_data <= 16'h0DAC;
        end
        //set the "busy" register.
        if (cmd_bus_addr[7:0] == 10) begin
          out_data <= {15'b0, busy};
        end     
      end else
        out_data <= 0;  //make sure to set this in case of troubles.
  end
end



//Control logic
reg [3:0] counter = 0;
reg shift_out_cmd_bus_enable;
reg load_shift_reg;
reg count_up;
reg count_res;
reg reset_ebi_captured_data;

reg load_command_bus_data;

reg load_last_executed_command_bus_data;


parameter init =  3'b001;
parameter load =  3'b010;
parameter pulse=  3'b100;
reg[2:0] state, nextState;

initial begin
  state = init;
end

always @ (posedge sclk) begin
  if (reset) state <= init;
  else state <= nextState;
end


always @ (*) begin
  nextState = 3'bXXX;
  load_shift_reg = 1'b0;
  shift_out_cmd_bus_enable = 1'b0;
  reset_command_bus_data = 1'b0;
  count_up = 1'b0;
  nLdac = 1'b1;
  nSync = 1'b1;

  case (state)

    //Idle here until we have a new value loaded in the command bus register
    init: begin
      nextState = init;
      count_res = 1'b1;

      if (command_bus_data[31:16] == DAC_CMD_NEW_VALUE) begin
        nextState = load;
        load_shift_reg = 1'b1;  
      end
    end

    //load a value into the DAC and hang around in this state until we have
    //shifted out 16 bits
    load: begin
      nextState = load;

      count_up = 1'b1;
      nSync = 1'b0;
      shift_out_cmd_bus_enable = 1'b1;

      if (counter == 15) begin
        reset_command_bus_data = 1'b1;
        nextState = pulse;
      end
    end

    //Give a load pulse to set up the DAC
    pulse: begin
      nextState = init;

      nLdac = 1'b0;
      count_res = 1'b1;
    end

  endcase
end


//Slowcmd_bus_data path
always @ (posedge sclk) begin
  if (reset) 
    shift_out_register <= 0;
  else begin

    //Reset counter if aSKED TO.
    if (count_res)
      counter <= 0;
    else 
      if (count_up)
        counter <= counter + 1;

      if (load_shift_reg)
        shift_out_register <= command_bus_data[15:0];
      else 
        if (shift_out_cmd_bus_enable)   //this should -in theory- empty out the shift register...
          shift_out_register <= {shift_out_register[14:0], 1'b0};
  end
end
assign dac_din = shift_out_register[15];

endmodule
