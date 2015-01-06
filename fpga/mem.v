  /* 
  * This module is the interface for sampling storage.
  *
  * It has room for 10 adresses which point to
  * the various units in the system. By writing the POSITION to address 0,
  * the collection_units register will be updated with this unit and it will
  * be included in the collection state machine.
  * 
  * When reading from this unit, a sample will be collected. If no reading is done,
  * the unit will simply keep filling the memory until it is full.
  */

  module mem (
    input clk,
    input rst,

    //EBI configuration interface
    input [18:0] addr
    input [15:0] ebi_data_in,
    output reg [15:0] ebi_data_out,
    input enable,
    input re,
    input rw,

    output reg [63:0] controller_enable,

    //Memory writing interface exposed to the mem-subsystem of the controllers.
    input [15:0] sample_data
  );

  parameter POSITION = 0;
  localparam ADDR_NEXT_SAMPLE = 1;

  //The collection unit registers hold the position of the unit to enable.
  reg [8:0] collection_units[15:0];
  reg [3:0] num_units = 0;

  reg [20:0] num_samples = 0;
  reg [20:0] memory_top_addr = 0;
  reg [20:0] memory_bottom_addr = 0;

  //will start out as 0.
  reg rd_d = 0;
  wire rd_transaction_done;

  wire controller_enable;
  assign controller_enable = (enable & (addr[18:8] == POSITION));

  always @ (posedge clk) begin
    if(controller_enable & wr) begin
      collection_units[num_units] <= ebi_data_in[7:0];
      num_units <= num_units + 1;
    end

    /*
      Every time we read from this register, there should be a new
      sample ready.
    */
    if (controller_enable & rd) begin
      if (addr[7:0] == ADDR_NEXT_SAMPLE) begin
        ebi_data_out <= ram_data_out; //collect what's on the output
      end

      if (addr[7:0] == ADDR_NUM_SAMPLES) begin
        ebi_data_out <= num_samples;
      end
    end

    //Only increment if we have seen a falling edge on read signal.
    if(rd_transaction_done) begin
      memory_bottom_addr <= memory_bottom_addr + 1;
    end 
  end

  //
  
  //-----------------------------------------------------------------------------------
  //rd falling edge detection to see if a transaction has finished
  always @ posedge(clk)
    if(rst)
      rd_d <= 1'b0;
    else
      rd_d <= rd;
  end
  assign rd_transaction_done <= rd_d & (~rd);
  //-----------------------------------------------------------------------------------


  //When a read to the special read register is done,
  //memory_bottom_addr is incremented and the next value is fetched from
  //memory automatically such that it is available when the next read request comes.
  
  //state machine that runs across all collection_units and fetches a sample,
  //and then stores it at memory_top_addr.

  parameter idle = 4'b0001;
  parameter store = 4'b0010;
  parameter increment = 4'b0100;
  parameter fetch = 4'b1000;
  reg [3:0] state;

  /* control path signals */
  wire inc_curr_id_idx;
  wire inc_num_samples;
  wire capture_sample_data;
  wire ram_write_enable;

  initial begin 
    state = idle;
  end


  reg [15:0] last_fetched [10:0];
  reg [5:0] current_id_idx = 0;


  always @ (posedge clk) begin
    if (reset) begin
      en_ram_write <= 1'b0;
      en_ram_read  <= 1'b0;
      inc_curr_id_idx <= 1'b0;
      inc_num_samples <= 1'b0;
      capture_sample_data <= 1'b0;
      ram_write_enable <= 1'b0;
    end

    case (state)
      idle: begin
        controller_enable[current_id_idx] <= 0;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;

        state <= fetch;
      end

      fetch: begin
        controller_enable[current_id_idx] <= 1'b1;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;

        state <= store;
      end

      store: begin
        controller_enable[current_id_idx] <= 0;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;

        if (last_fetched[current_id_idx] != sample_data) begin
          ram_write_enable <= 1'b1;
        end

        state <= fetch; 
      end

      increment: begin
        controller_enable[current_id_idx] <= 0;
        inc_curr_id_idx <= 1'b1;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;

        state <= fetch;
      end

      default: begin
        controller_enable[current_id_idx] <= 0;
        inc_curr_id_idx <= 1'b0;
        inc_num_samples <= 1'b0;
        capture_sample_data <= 1'b0;
        ram_write_enable <= 1'b0;

        state <= idle;
      end

    endcase
  end


  always @ (posedge clk) begin
    
   
    if inc_curr_id_idx 
      current_id_idx <= (current_id_idx + 1) % num_units;

    if inc_num_samples
      num_samples <= num_samples + 1;
    
    if capture_sample_data
      last_fetched[current_id_idx] <= sample_data;

  end


  wire [15:0] ram_data_out;
  wire [15:0] ram_data_in;
  dp_ram(
    .clk(clk),
    .ena(1'b1),
    .enb(1'b1),
    .wea(ram_write_enable),
    .addra(memory_top_addr),
    .addrb(memory_bottom_addr),
    .dia(ram_data_in),
    .doa(ram_data_out),
    .dob()
  );


endmodule;
