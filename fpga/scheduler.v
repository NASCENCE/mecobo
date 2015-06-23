module scheduler (	input 			clk,
			input 			rst,

			//interface to timer.
			input 	[31:0]		current_time,
			output reg		reset_time,

			//cmd fifo
			input [95:0]		cmd_fifo_dout,
			input			cmd_fifo_empty,
			input			cmd_fifo_valid,
			output reg		cmd_fifo_rd_en,
			//dac fifo
			input [15:0]		dac_fifo_dout,
			input			dac_fifo_empty,
			output			dac_fifo_rd_en,

			//External bus interface to all the chippies.
			output 	[18:0] 		cmd_bus_addr,
			output 	[15:0] 		cmd_bus_data,
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

always @ ( * ) begin
	nextState = 4'bXXXX;
	cmd_fifo_rd_en = 1'b0;
	cmd_bus_wr = 1'b0;
	cmd_bus_rd = 1'b0;
	cmd_bus_en = 1'b0;	
	writeCommandReg = 1'b0;
	resetCommandReg = 1'b0;
	reset_time = 1'b0;
	case (state)
		idle: begin
			nextState = fetch;
			reset_time = 1'b1;	
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
			if (current_time >= command[63:32]) begin
				cmd_bus_wr = 1'b1;
				cmd_bus_en = 1'b1;
				resetCommandReg = 1'b1;
				nextState = fetch;	
			end
		end
					
	endcase
end
	

// |    32 bits: TIME     | 4 bits: CMD  |  20 bits: DATA | 
//datapath
localparam CTRL_H = 31;  //nibble 1 .. 2
localparam CTRL_L = 24;  //
localparam CMD_H = 23;   //nibble 3
localparam CMD_L = 20;   
localparam DATA_H = 19;  //5 last nibblets 
localparam DATA_L = 0;

reg [63:0] command = 0;

//format the address 
assign cmd_bus_addr[15:0] = {command[CTRL_H:CTRL_L],4'b0000,command[CMD_H:CMD_L]};
assign cmd_bus_data = command[DATA_H:DATA_L];

always @ (posedge clk) begin
	if (resetCommandReg) 
		command <= 0;
	else
		if (writeCommandReg & cmd_fifo_valid) begin
			command <= cmd_fifo_dout;
		end
end


endmodule
