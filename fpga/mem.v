  /* 
  * This module is the interface for sampling storage.
  *
  * It has room for 10 adresses which point to
  * the various units in the system. By writing the POSITION to address 0,
  * the collection_channels register will be updated with this unit and it will
  * be included in the collection state machine.
  * 
  * When reading from this unit, a sample will be collected. If no reading is done,
  * the unit will simply keep filling the memory until it is full.
  */

  module mem (
    input clk,
    input rst,

    //EBI configuration interface
    input [18:0] addr,
    input [15:0] ebi_data_in,
    output reg [15:0] ebi_data_out,
    input cs,
    input re,
    input wr,

    output reg output_sample,
    output [7:0] channel_select,

    //Memory writing interface exposed to the mem-subsystem of the controllers.
    input [31:0] sample_data
);

parameter POSITION = 242;
parameter MAX_SAMPLES = 65533;
parameter MAX_COLLECTION_UNITS = 16;

localparam ADDR_NEXT_SAMPLE = 1;
localparam ADDR_NUM_SAMPLES = 2;
localparam ADDR_READ_ADDR = 3;
localparam ADDR_NEW_UNIT = 4;
localparam ADDR_LOCAL_COMMAND = 5;
localparam ADDR_NUM_UNITS = 6;
localparam ADDR_DEBUG = 10;

localparam
CMD_START_SAMPLING = 1,
CMD_STOP_SAMPLING = 2,
CMD_RESET = 5;


//The collection unit registers hold the position of the unit to enable.
reg [7:0] collection_channels[0:15];
reg [3:0] num_units = 0;

reg [18:0] num_samples = 0;
reg [18:0] memory_top_addr = 0;
reg [18:0] memory_bottom_addr = 0;

reg [15:0] command;
//will start out as 0.
wire rd_transaction_done;
wire wr_transaction_done;

wire controller_enable;
assign controller_enable = (cs & (addr[15:8] == POSITION));


reg [15:0] ebi_captured_data;
reg new_unit_added;
reg sample_fetched;

//EBI capture
always @ (posedge clk) begin
  if(rst) begin
    ebi_data_out <= 0;
    ebi_captured_data <= 0;
    sample_fetched <= 0;
    new_unit_added <= 0;
  end else begin
    if (res_cmd_reg) 
      command <= 0;
    else begin
      if (controller_enable & wr) begin
        if (addr[7:0] == ADDR_NEW_UNIT) begin
          ebi_captured_data <= ebi_data_in;
	end
 
        if (addr[7:0] == ADDR_LOCAL_COMMAND) begin
          command <= ebi_data_in;
        end

      end       
    end //if res_cmd_reg end
  
  /*
    Every time we read from this register, there should be a new
    sample ready.
  */
    if (controller_enable & re) begin
      if ((addr[7:0] == ADDR_NEXT_SAMPLE)) begin
        ebi_data_out <= ram_data_out;
      end      
      if (addr[7:0] == ADDR_NUM_SAMPLES) begin
        ebi_data_out <= num_samples;
      end

      if (addr[7:0] == ADDR_NUM_UNITS) begin
        ebi_data_out <= num_units;
      end

      if (addr[7:0] == ADDR_DEBUG) begin
        ebi_data_out <= 16'hBAAB;
      end

      if (addr[7:0] == ADDR_READ_ADDR) begin
        ebi_data_out <= memory_bottom_addr;
      end

    end //if re end
    else begin
      ebi_data_out <= 0;
    end
  end //rst end
end

//

//-----------------------------------------------------------------------------------
//rd falling edge detection to see if a transaction has finished


reg rd_d = 0;
reg rd_dd = 0;
reg wr_d = 0;
reg wr_dd = 0;

always @ (posedge clk) begin
  if(rst) begin
    rd_d <= 1'b0;
    wr_d <= 1'b0;
    rd_dd <= 1'b0;
    wr_dd <= 1'b0;
  end else begin
    rd_d <= re & controller_enable;
    rd_dd <= rd_d;
    wr_d <= wr & controller_enable;
    wr_dd <= wr_d;
  end 
end

//if r_dd is high and rd_d is low, we have seen a falling edge.
assign rd_transaction_done = (~rd_d) & (rd_dd);
assign wr_transaction_done = (~wr_d) & (wr_dd);


//-----------------------------------------------------------------------------------


//When a read to the special read register is done,
//memory_bottom_addr is incremented and the next value is fetched from
//memory automatically such that it is available when the next read request comes.

//state machine that runs across all collection_channels and fetches a sample,
//and then stores it at memory_top_addr.

parameter idle = 5'b00001;
parameter store = 5'b00010;
parameter increment = 5'b00100;
parameter fetch = 5'b01000;
parameter fetch_wait = 5'b10000;
reg [4:0] state;

/* control path signals */
reg inc_curr_id_idx;
reg inc_num_samples;
reg ram_write_enable;
reg en_read;
reg res_cmd_reg;
reg res_sampling;
reg capture_sample_data;

reg [31:0] last_fetched [0:15];
reg [31:0] sample_data_reg;

integer mi;
initial begin 
  state = idle;
  for (mi = 0; mi < MAX_COLLECTION_UNITS; mi = mi + 1)  begin
    collection_channels[mi] = 255;
    last_fetched[mi] = 0;
  end
    

end


reg [3:0] current_id_idx = 0;


always @ (posedge clk) begin
  if (rst) begin
    inc_curr_id_idx <= 1'b0;
    inc_num_samples <= 1'b0;
    ram_write_enable <= 1'b0;
    en_read <= 1'b0;
    res_cmd_reg <= 1'b1;
    res_sampling <= 1'b1;
    capture_sample_data <= 1'b0;

    state <= idle;
  end

  case (state)
    idle: begin
      output_sample <= 1'b0;
      capture_sample_data <= 1'b0;
      inc_curr_id_idx <= 1'b0;
      inc_num_samples <= 1'b0;
      ram_write_enable <= 1'b0;
      en_read <= 1'b1;
      res_cmd_reg <= 1'b0;
      res_sampling <= 1'b0;

      if(command == CMD_START_SAMPLING) begin
      	res_cmd_reg <= 1'b1;
        state <= fetch;
      end else if (command == CMD_RESET) begin
        res_cmd_reg <= 1'b1;
        res_sampling <= 1'b1;
        state <= idle;
      end else
        state <= idle;
    end

    //instruct adc to output new sample data
    fetch: begin
      output_sample <= 1'b1;
      capture_sample_data <= 1'b0;
      inc_curr_id_idx <= 1'b0;
      inc_num_samples <= 1'b0;
      ram_write_enable <= 1'b0;
      en_read <= 1'b1;
      res_cmd_reg <= 1'b0;
      res_sampling <= 1'b0;

      state <= fetch_wait;
    end
   
    //give the adc time to set up the new sample data 
    fetch_wait: begin
      output_sample <= 1'b1;
      capture_sample_data <= 1'b0;
      inc_curr_id_idx <= 1'b0;
      inc_num_samples <= 1'b0;
      ram_write_enable <= 1'b0;
      en_read <= 1'b1;
      res_cmd_reg <= 1'b0;
      res_sampling <= 1'b0;

      state <= store;
    end

    //capture newly set up sample data
    store: begin
      output_sample <= 1'b0;
      capture_sample_data <= 1'b1;
      inc_curr_id_idx <= 1'b0;
      inc_num_samples <= 1'b0;
      ram_write_enable <= 1'b0;
      en_read <= 1'b1;
      res_cmd_reg <= 1'b0;
      res_sampling <= 1'b0;

      
      state <= increment; 
    end

    //increment index counters
    increment: begin
      output_sample <= 1'b0;
      capture_sample_data <= 1'b0;
      inc_curr_id_idx <= 1'b1;
      inc_num_samples <= 1'b0;
      ram_write_enable <= 1'b0;
      en_read <= 1'b1;
      res_cmd_reg <= 1'b0;
      res_sampling <= 1'b0;

      if (last_fetched[current_id_idx] != sample_data_reg) begin
        ram_write_enable <= 1'b1;
      	inc_num_samples <= 1'b1;
      end

      if(command == CMD_RESET)  begin
	res_sampling <= 1'b1;
        res_cmd_reg <= 1'b1;
        state <= idle;
      end else if (command == CMD_STOP_SAMPLING) begin
        res_cmd_reg <= 1'b1;
        state <= idle;
      end else
        state <= fetch;
    end

    default: begin
      output_sample <= 1'b0;
      capture_sample_data <= 1'b0;
      inc_curr_id_idx <= 1'b0;
      inc_num_samples <= 1'b0;
      ram_write_enable <= 1'b0;
      en_read <= 1'b0;
      res_cmd_reg <= 1'b1;
      res_sampling <= 1'b1;
      
      state <= idle;
    end

  endcase
end

assign channel_select = collection_channels[current_id_idx];

reg [15:0] debugcounter = 0;


integer mj;
always @ (posedge clk) begin


  if(res_sampling == 1'b1) begin
    num_samples <= 0;
    num_units <= 0;
    memory_top_addr <= 0;
    memory_bottom_addr <= 0;
    current_id_idx <= 0;
    sample_data_reg <= 0;

    for (mj = 0; mj < MAX_COLLECTION_UNITS; mj = mj + 1)  begin
      collection_channels[mj] <= 255; //no such channel: special.
      last_fetched[mj] <= 0;
    end
 
  end else begin
    if (inc_num_samples) begin
      if (num_samples < MAX_SAMPLES) begin
        num_samples <= num_samples + 1;
        memory_top_addr <= memory_top_addr + 1;
      end
    end

    if (wr_transaction_done & (addr[7:0] == ADDR_NEW_UNIT)) begin
      collection_channels[num_units] <= ebi_captured_data[7:0];
      num_units <= num_units + 1;
    end

    //Only increment if we have seen a falling edge on read signal.
    if (rd_transaction_done & (addr[7:0] == ADDR_NEXT_SAMPLE)) begin
      if(memory_bottom_addr < MAX_SAMPLES)
        memory_bottom_addr <= memory_bottom_addr + 1;
    end 

    if(capture_sample_data) begin
      sample_data_reg <= sample_data;
    end
  
   if(ram_write_enable) begin
     last_fetched[current_id_idx] <= sample_data_reg;
   end
	 
    //only increment mod num_units-1 (0-indexed)
    if (inc_curr_id_idx) begin
      if(current_id_idx == (num_units-1)) 
        current_id_idx <= 0;
      else
        current_id_idx <= (current_id_idx + 1);
    end

  end
end


wire [15:0] ram_data_out;
wire [15:0] ram_data_in;

assign ram_data_in = {current_id_idx[2:0], sample_data_reg[12:0]}; //last_fetched[current_id_idx][15:0];

dp_ram sample_ram(
  .clk(clk),
  .ena(ram_write_enable),
  .enb(en_read),
  .wea(ram_write_enable),
  .addra(memory_top_addr),
  .addrb(memory_bottom_addr),
  .dia(ram_data_in),
  .doa(),
  .dob(ram_data_out)
);

endmodule
