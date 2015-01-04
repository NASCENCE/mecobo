  /* 
  * This module is the interface for sampling storage.
  *
  * It has room for 10 adresses which point to
  * the various units in the system. 
  * 
  * The unit has a configuration port shared with all the other units in the
  * system. When set up, it will round-robin through the addresses and allow
  * the units to write into sample memory if it has a new sample available.
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

    //controller enable -- each pin controller will have one bit here.
    //TODO: Somehow decode the address to 1 bit here.
    output reg [50:0] controller_enable,

    //Memory writing interface exposed to the mem-subsystem of the controllers.
    input write_enable,
    input [15:0] sample_data
  );

  parameter POSITION = 0;

  //The collection unit registers hold the position of the unit to enable.
  reg [15:0] collection_units[9:0];
  reg [3:0] num_units = 0;

  wire controller_enable;
  assign controller_enable = (enable & (addr[18:8] == POSITION));

  //If this unit is selected, grab the data.
  always @ (posedge clk) begin
    if(controller_enable & wr) begin
      collection_units[addr[3:0]] <= ebi_data_in;
      num_units <= num_units +1;
    end
  end

  //state machine that runs across all collection_units and fetches a sample,
  //puts it into memory.
  //TODO.


endmodule;
