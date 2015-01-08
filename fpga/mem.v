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
    output reg [7:0] channel_select,

    //Memory writing interface exposed to the mem-subsystem of the controllers.
    input [15:0] sample_data
  );

  parameter POSITION = 242;

  localparam ADDR_NEXT_SAMPLE = 1;
  localparam ADDR_NUM_SAMPLES = 2;

  //The collection unit registers hold the position of the unit to enable.
  reg [7:0] collection_channels[0:15];
  reg [3:0] num_units = 0;

  reg [18:0] num_samples = 0;
  reg [18:0] memory_top_addr = 0;
  reg [18:0] memory_bottom_addr = 0;

  //will start out as 0.
  reg rd_d = 0;
  wire rd_transaction_done;

  wire controller_enable;
  assign controller_enable = (cs & (addr[18:8] == POSITION));

  always @ (posedge clk) begin
    if (controller_enable & wr) begin
      collection_channels[num_units] <= ebi_data_in[7:0];
      num_units <= num_units + 1;
    end

    /*
      Every time we read from this register, there should be a new
      sample ready.
    */
    if (controller_enable & re) begin
      if (addr[7:0] == ADDR_NEXT_SAMPLE) begin
        ebi_data_out <= ram_data_out; //collect what's on the output
      end

      if (addr[7:0] == ADDR_NUM_SAMPLES) begin
        ebi_data_out <= num_samples;
      end
    end

    //Only increment if we have seen a falling edge on read signal.
    if (rd_transaction_done) begin
      memory_bottom_addr <= memory_bottom_addr + 1;
    end 
  end

  //
  
  //-----------------------------------------------------------------------------------
  //rd falling edge detection to see if a transaction has finished
    always @ (posedge clk) begin
    if(rst)
      rd_d <= 1'b0;
    else
      rd_d <= re;
  end
  assign rd_transaction_done = rd_d & (~re);
  //-----------------------------------------------------------------------------------


  //When a read to the special read register is done,
  //memory_bottom_addr is incremented and the next value is fetched from
  //memory automatically such that it is available when the next read request comes.
  
  //state machine that runs across all collection_channels and fetches a sample,
  //and then stores it at memory_top_addr.

  parameter idle = 4'b0001;
  parameter store = 4'b0010;
  parameter increment = 4'b0100;
  parameter fetch = 4'b1000;
  reg [3:0] state;

  /* control path signals */
  reg inc_curr_id_idx;
  reg inc_num_samples;
  reg capture_sample_data;
  reg ram_write_enable;
  reg en_read;
  reg en_write;

  initial begin 
    state = idle;
  end


  reg [15:0] last_fetched [0:15];
  reg [3:0] current_id_idx = 0;


  always @ (posedge clk) begin
    if (rst) begin
      inc_curr_id_idx <= 1'b0;
      inc_num_samples <= 1'b0;
      capture_sample_data <= 1'b0;
      ram_write_enable <= 1'b0;
      en_read <= 1'b0;
      en_write <= 1'b0;

      state <= idle;
    end

    case (state)
      idle: begin
        output_sample <= 1'b0;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;
        en_read <= 1'b1;
        en_write <= 1'b0;


        state <= fetch;
      end

      fetch: begin
        output_sample <= 1'b1;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;
        en_read <= 1'b1;
        en_write <= 1'b0;



        state <= store;
      end

      store: begin
        output_sample <= 1'b0;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;
        en_read <= 1'b1;
        en_write <= 1'b1;


        if (last_fetched[current_id_idx] != sample_data) begin
          ram_write_enable <= 1'b1;
        end

        state <= increment; 
      end

      increment: begin
        output_sample <= 1'b0;
        inc_curr_id_idx <= 1'b1;
        inc_num_samples <= 1'b1;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;
        en_read <= 1'b1;
        en_write <= 1'b0;


        state <= fetch;
      end

      default: begin
        output_sample <= 1'b0;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;
        en_read <= 1'b1;
        en_write <= 1'b0;
        
        state <= idle;
      end

    endcase
  end


  always @ (posedge clk) begin
    
    channel_select <= collection_channels[current_id_idx];
   
    if (inc_curr_id_idx) begin
      current_id_idx <= (current_id_idx + 1);
    end

    if (inc_num_samples) begin
      num_samples <= num_samples + 1;
      memory_top_addr <= memory_top_addr + 1;
    end
    
    if (capture_sample_data)
      last_fetched[current_id_idx] <= sample_data;

  end


    wire [15:0] ram_data_out;
  wire [15:0] ram_data_in;
  assign ram_data_in = last_fetched[current_id_idx];

  dp_ram sample_ram(
    .clk(clk),
    .ena(ram_write_enable),
    .enb(1'b1),
    .wea(ram_write_enable),
    .addra(memory_top_addr),
    .addrb(memory_bottom_addr),
    .dia(ram_data_in),
    .doa(ram_data_out),
    .dob()
  );

endmodule
