module scheduler (	input 			clk,
input 			rst,

//interface to timer.
input 	[31:0]		current_time,

//cmd fifo
input [79:0]		cmd_fifo_dout,
input			cmd_fifo_empty,
input			cmd_fifo_valid,
output reg		cmd_fifo_rd_en,
//dac fifo
input [15:0]		dac_fifo_dout,
input			dac_fifo_empty,
output			dac_fifo_rd_en,

//External bus interface to all the chippies.
output 	[15:0] 		cmd_bus_addr,
output 	[31:0] 		cmd_bus_data,
output	reg		cmd_bus_en,
output	reg		cmd_bus_rd,
output	reg		cmd_bus_wr
);


localparam [4:0] fetch 		= 5'b00010,
	         fifo_wait	= 5'b00100,
	         exec		= 5'b01000,
	         idle		= 5'b10000,
                 exec_wait   	= 5'b00001;

//control section state machine.
reg [4:0] state, nextState;

always @ (posedge clk or posedge rst)
  if (rst) state <= idle;
  else state <= nextState;


  //output and next state logic.
  //note that outputs are not registered.


  reg writeCommandReg = 1'b0;
  reg resetCommandReg = 1'b0;

  // |    32 bits: TIME     | 16 bits: internal bus ADDR  |  32 bits: internal
  // bus DATA | 
  //datapath
  localparam TIME_H = 79;  //32 bitties
  localparam TIME_L = 48;
  localparam DATA_H = 47;  //32 bitties...ties? uh. what.
  localparam DATA_L = 16;  //
  localparam ADDR_H = 15;  //5 last nibblets 
  localparam ADDR_L = 0;

  reg [79:0] command = 0;

  always @ ( * ) begin
    nextState = 4'bXXXX;
    cmd_fifo_rd_en = 1'b0;
    cmd_bus_wr = 1'b0;
    cmd_bus_rd = 1'b0;
    cmd_bus_en = 1'b0;	
    writeCommandReg = 1'b0;
    resetCommandReg = 1'b0;
    case (state)
      idle: begin
        nextState = idle; 
        //If time has started running, it's time to start scheduling stuff!
        if (current_time != 0) 
          nextState = fetch;
      end

      fetch: begin
        if (cmd_fifo_empty | (current_time == 0)) begin   //idle while waiting for time to tick or if fifo is empty
          nextState = fetch;
        end
        else begin
          cmd_fifo_rd_en = 1'b1;  //tell fifo to output data
          nextState = fifo_wait;
        end

      end

      fifo_wait: begin
        nextState = fifo_wait;
        if(cmd_fifo_valid) begin
          writeCommandReg = 1'b1;   //fetch data on the coming flank.
          nextState = exec;
        end
      end

      //command reg is written and now 
      exec: begin
        nextState = exec; 

        if (current_time >= command[TIME_H:TIME_L]) begin
          cmd_bus_wr = 1'b1;
          cmd_bus_en = 1'b1;
          nextState = exec_wait;	
        end
      end

      exec_wait: begin
        nextState = fetch; 
        //We will keep signals high here to allow capture of data in the 
        //pincontrol modules AND allow a command-reset.
        //See also comment in pincontrol.v
        //Note that we don't write here, 
        //we just give the machine time to run 1 cycle for an eventual reset.
        //[we could avoid this, the signals are stable]
      end


    endcase
  end


  //format the address 
  assign cmd_bus_addr[15:0] = command[ADDR_H:ADDR_L];
  assign cmd_bus_data = command[DATA_H:DATA_L];

  always @ (posedge clk) begin
    if (resetCommandReg) 
      command <= 0;
    else
      if (writeCommandReg) begin
        command <= cmd_fifo_dout;
      end
  end


endmodule
