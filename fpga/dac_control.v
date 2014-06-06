module dac_control (
input ebi_clk, //System clock
input sclk, //Max 30MHz
input reset,
input enable,
input re,
input wr,
input [15:0] data,
output reg [15:0] out_data,
input [18:0] addr,
//facing the DAC
output reg nLdac,
output reg nSync,
output dac_din);

parameter POSITION = 0;


//Controller select (not to be confused with chip select)
wire cs;
assign cs = (enable & (addr[18:8] == POSITION));
wire busy;
//assign busy = (state == load) | (state==pulse);

//EBI data capture - technically also part of the data path.
reg [15:0] ebi_captured_data = 0;
always @ (posedge ebi_clk) begin
  if (reset) begin
    out_data <= 16'h0000;
  end
  else begin
        //Capture or reset the capture register.
    //if (reset_ebi_captured_data)
    //  ebi_captured_data <= 0;
    if (cs & wr)
      ebi_captured_data <= data;
    
    //Driving out data 
    if (cs & re) begin
      if (addr[7:0] == 9) begin
        out_data <= 16'h0DAC;
      end
      //set the "busy" register.
      if (addr[7:0] == 10) begin
        out_data <= {15'b0, busy};
      end     
      if (addr[7:0] == 11) begin
        out_data <= last_executed_command;
      end
    end else
      out_data <= 0;  //make sure to set this in case of troubles.
  end
end



//Control logic
reg [3:0] counter = 0;
reg shift_out_enable;
reg load_shift_reg;
reg count_up;
reg count_res;
reg reset_ebi_captured_data;

reg load_current_command;
reg reset_current_command;

reg load_last_executed_command;

 
parameter init =  3'b001;
parameter load =  3'b010;
parameter pulse=  3'b100;
reg[2:0] state;

initial begin
  state = init;
end

always @ (posedge sclk) begin
  if (reset) begin
        state <= init;
        load_shift_reg <= 1'b0;
        shift_out_enable <= 1'b0;
        reset_ebi_captured_data <= 1'b1;
        count_up <= 1'b0;
        count_res <= 1'b1;
        nLdac <= 1'b1;
        nSync <= 1'b1;
  end else begin
    case (state)
     
      init: begin
        state <= init;
        reset_ebi_captured_data <= 1'b0;
        shift_out_enable <= 1'b0;
        count_up <= 1'b0;
        count_res <= 1'b1;
        nLdac <= 1'b1;
        nSync <= 1'b1;
        load_shift_reg <= 1'b0;

        load_current_command <= 1'b1;
        reset_current_command <= 1'b0;

        load_last_executed_command <= 1'b0;

        if (current_command != 0) begin
          state <= load;
          load_shift_reg <= 1'b1;
        end
      end

      //load a value into mr. dac
      load: begin
        state <= load;
        load_shift_reg <= 1'b0;
        reset_ebi_captured_data <= 1'b0;
        shift_out_enable <= 1'b1;
        count_up <= 1'b1;
        count_res <= 1'b0;
        nLdac <= 1'b1;  
        nSync <= 1'b0;  

        load_current_command <= 1'b0;
        reset_current_command <= 1'b0;

        load_last_executed_command <= 1'b0;


        if (counter == 15) begin
          state <= pulse;
          nSync <= 1'b1;
          nLdac <= 1'b0;
          load_last_executed_command <= 1'b1; //synchronize
        end
      end

      pulse: begin
        load_shift_reg <= 1'b0;
        reset_ebi_captured_data <= 1'b1;
        shift_out_enable <= 1'b0;
        count_up <= 1'b0;
        count_res <= 1'b0;
        nLdac <= 1'b1;  
        nSync <= 1'b1;  
        state <= init;

        load_current_command <= 1'b0;
        reset_current_command <= 1'b1;  //reset current command before entering init. might have new stuff soon!
        load_last_executed_command <= 1'b0;

      end

      default: begin
        state <= init;
        load_shift_reg <= 1'b0;
        shift_out_enable <= 1'b0;
        reset_ebi_captured_data <= 1'b0;
        count_up <= 1'b0;
        count_res <= 1'b0;
        nLdac <= 1'b0;
        nSync <= 1'b0;

        load_current_command <= 1'b0;
        reset_current_command <= 1'b0;

        load_last_executed_command <= 1'b0;

      end
    endcase
  end
end


//Slow data path
reg [15:0] shift_out_register = 0;
reg [15:0] current_command = 0;
reg [15:0] last_executed_command = 0;

always @ (posedge sclk) begin
  if (reset) 
    shift_out_register <= 0;
  else begin
   
    if (load_last_executed_command) begin
      last_executed_command <= current_command;
    end

    if (reset_current_command) begin
      current_command <= 0;
    end else if (load_current_command) begin
      current_command <= ebi_captured_data;
    end

    //Reset counter if aSKED TO.
    if (count_res)
      counter <= 0;
    else 
      if (count_up)
        counter <= counter + 1;

    if (load_shift_reg)
      shift_out_register <= ebi_captured_data;
    else 
      if (shift_out_enable)   //this should -in theory- empty out the shift register...
        shift_out_register <= {shift_out_register[14:0], 1'b0};
  end
end
assign dac_din = shift_out_register[15];

endmodule
