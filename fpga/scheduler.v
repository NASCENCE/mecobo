module scheduler (	input clk,
			input rst,

			//interface to timer.
			input [31:0] current_time,
			output reset_time,

			//cmd fifo
			input [63:0]	cmd_fifo_dout,
			input		cmd_fifo_empty,
			input		cmd_fifo_valid,
			output reg	cmd_fifo_rd_en,
			//dac fifo
			input [15:0]	dac_fifo_dout,
			input		dac_fifo_empty,
			output		dac_fifo_rd_en

			//External bus interface to all the chippies.
			output 	[18:0] 	cmd_bus_addr,
			output 	[15:0] 	cmd_bus_data,
			output		cmd_bus_en,
			output		cmd_bus_rd,
			output		cmd_bus_wr
);


//control section state machine.
reg [3:0] state, nextState;

always @ (posedge clk or posedge rst)
	if (rst) state <= idle;
	else state <= nextState;


//output and next state logic.
//note that outputs are not registered.
always @ ( * ) begin
	nextState = 4'bXXXX;
	cmd_fifo_rd_en <= 1'b0;
	cmd_bus_wr = 1'b0;
	cmd_bus_rd = 1'b0;
	cmd_bus_en = 1'b0;	
	writeCommandReg = 1'b0;
	case (state)
		fetch: begin
			nextState = fetch;
			if (current_time >= command) begin
				cmd_fifo_rd_en = 1'b1;
				nextState = exec;	
			end
		exec: begin
			writeCommandReg = 1'b1;
			nextState = bus;
		end

		bus: begin
			cmd_bus_wr = 1'b1;
			cmd_bus_rd = 1'b0;
			cmd_bus_en = 1'b1;
			nextState = fetch;
		end
					
	endcase
end
	


//datapath

localparam CTRL_H = 31;
localparam CTRL_L = 24;
localparam CMD_H = 23;
localparam CMD_L = 20;
localparam DATA_H = 19;
localparam DATA_L = 0;

reg [63:0] command = 0;

assign cmd_bus_addr[15:0] <= {command[CTRL_H:CTRL_L],4'b0,command[CMD_H:CMD_L]};
assign cmd_bus_data <= command[DATA_H:DATA_L];

always @ (posedge clk) begin
	if (writeCommandReg & cmd_fifo_valid) begin
		command <= cmd_fifo_dout;
	end
end


endmodule
