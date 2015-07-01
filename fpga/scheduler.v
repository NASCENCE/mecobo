module scheduler (	input 			clk,
			input 			rst,

			//interface to timer.
			input 	[31:0]		current_time,
			output 			reset_time,

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
			output 	[18:0] 		cmd_bus_addr,
			output 	[31:0] 		cmd_bus_data,
			output	reg		cmd_bus_en,
			output	reg		cmd_bus_rd,
			output	reg		cmd_bus_wr
);


localparam [3:0] 	fetch 		= 4'b0000,
			fifo_wait	= 4'b0001,
			exec		= 4'b0010,
			idle		= 4'b0100;

//control section state machine.
reg [3:0] state, nextState;

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
			nextState = fetch;
		end

		fetch: begin
			if (cmd_fifo_empty) begin
				nextState = fetch;
			end
			else begin
				cmd_fifo_rd_en = 1'b1;  //tell fifo to output data
				nextState = fifo_wait;
			end

		end

		fifo_wait: begin
			nextState = exec;
			writeCommandReg = 1'b1;   //fetch data on the coming flank.
		end

		exec: begin
			nextState = exec;
			if ((command[TIME_H:TIME_L] == 0) | (current_time >= command[TIME_H:TIME_L])) begin
				cmd_bus_wr = 1'b1;
				cmd_bus_en = 1'b1;
				resetCommandReg = 1'b1;
				nextState = fetch;	
			end
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
		if (writeCommandReg & cmd_fifo_valid) begin
			command <= cmd_fifo_dout;
		end
end

//Reseting timer
assign reset_time = (cmd_bus_addr[15:0] == 16'hFFFF) ? 1'b1 : 1'b0;

endmodule
