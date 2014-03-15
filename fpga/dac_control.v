module dac_control (
input ebi_clk, //System clock
input sclk, //Max 30MHz
input reset,
input re,
input wr,
input [15:0] data,
input [20:0] addr,
//facing the DAC
output reg nLdac,
output reg nSync,
output dac_din);

parameter POSITION = 0;


//Chip select
reg cs;
always @ (posedge ebi_clk)  begin
  if (addr[7:0] == POSITION) 
    cs <= 1;
  else 
    cs <= 0;
end

//EBI data capture
reg [15:0] ebi_captured_data = 0;
always @ (posedge ebi_clk) begin
  if (reset)
    ebi_captured_data <= 0;
  else begin
    if (cs & wr)
      ebi_captured_data <= data;
  end
end




//Control logic
reg [3:0] counter = 0;
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
        nLdac <= 1'b1;
        nSync <= 1'b1;
  end else begin
    case (state)
      //Idle / init, waiting for something to do.
      init: begin
        state <= init;

        shift_out_enable <= 1'b0;
        count_up <= 1'b0;
        count_res <= 1'b1;
        nLdac <= 1'b1;
        nSync <= 1'b1;

        if (ebi_captured_data != 0) begin
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
        nLdac <= 1'b0;
        nSync <= 1'b0;

        if (counter == 15) begin
          state <= init;
        end
      end
    endcase
  end
end


//Data path
reg [15:0] shift_out_register = 0;
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
        shift_out_register <= {shift_out_register[14:0], 1'b0};
  end
end
assign dac_din = shift_out_register[15];

endmodule
