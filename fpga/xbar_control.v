module xbar_control (
input ebi_clk, //System clock
input sclk, //Max 30MHz
input reset,
input re,
input wr,
input [15:0] data,
input [18:0] addr,
//facing the DAC
output reg xbar_clock_enable,
output reg pclk, //assumption: Keep this low. Then make it high; shift data, then give low pulse...hm. 
output sin);

parameter POSITION = 0;

//Chip select
reg cs;
always @ (posedge ebi_clk)  begin
  if (addr[7:0] == POSITION) 
    cs <= 1;
  else 
    cs <= 0;
end

localparam 
  LOAD = 1'b1;

//EBI data capture
reg [255:0] ebi_captured_data = 0;
reg [15:0] command = 0;
always @ (posedge ebi_clk) begin
  if (reset)
    ebi_captured_data <= 0;
  else begin
    if (cs & wr)
      //If address 13 is high, then we have a command.
      if (addr[13]) 
        command <= data;
      else
        ebi_captured_data[addr[12:8]] <= data;
  end
end


//Control logic
reg [7:0] counter = 0;  //clock out 255 pulses.
reg shift_out_enable;
reg load_shift_reg;
reg count_up;
reg count_res;

parameter init = 1'b0;
parameter load = 1'b1;
reg state;

initial begin
  state = init;
end

always @ (posedge sclk) begin
  if (reset) begin
        state <= init;
        shift_out_enable <= 1'b0;
        count_up <= 1'b0;
        count_res <= 1'b1;
        xbar_clock_enable <= 1'b0;
        pclk <= 1'b1;
  end else begin
    //State machine start.
    case (state)
      //Idle / init, waiting for something to do.
      init: begin
        state <= init;

        shift_out_enable <= 1'b0;
        count_up <= 1'b0;
        count_res <= 1'b1;
        xbar_clock_enable <= 1'b0;
        pclk <= 1'b1;

        if (command == LOAD) begin
          state <= load;
          load_shift_reg = 1'b1;
        end
      end

      //load a value into mr. dac
      load: begin
        state <= load;
        
        shift_out_enable <= 1'b1;
        count_up <= 1'b1;
        count_res <= 1'b0;
        xbar_clock_enable <= 1'b1;
        pclk <= 1'b1;

        if (counter == 255) begin
          state <= init;
          pclk <= 1'b0; //give a pulse.
        end
      end
    endcase
  end
end


//Data path
reg [255:0] shift_out_register = 0;
always @ (posedge sclk) begin
  if (reset) 
    shift_out_register <= 0;
  else begin
    //Reset counter if asked to.
    if (count_res)
      counter <= 0;
    else 
      if (count_up)
        counter <= counter + 1;

    if (load_shift_reg)
      shift_out_register <= ebi_captured_data;
    else 
      if (shift_out_enable)
        shift_out_register <= {shift_out_register[254:0], 1'b0};
  end
end
assign sin = shift_out_register[255];

endmodule
