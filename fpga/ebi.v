module ebi(	input clk,
		input rst,
		//External interface to the world
		input [15:0] data_in,
		output [15:0] data_out,
		input [20:0] addr,
		input rd,
		input wr,
		input cs,
		//Interface to CMD fifo
		output 	[64:0] 	cmd_fifo_data_in,   // input into cmd FIFO
		output 		cmd_fifo_wr_en,
		input  		cmd_fifo_almost_full, 
		input  		cmd_fifo_full,
		input  		cmd_fifo_almost_empty,
		input  		cmd_fifo_empty,
		//Interace from SAMPLE DATA fifo
		input 	[15:0] 	sample_fifo_data_out,   //data FROM sample fifo
		output		sample_fifo_rd_en,
		input 		sample_fifo_almost_full,
		input		sample_fifo_full,
		input 		sample_fifo_almost_empty,
		input		sample_fifo_empty
		//TODO: DAC buffers.
);	

// ------------- EBI INTERFACE -----------------

localparam EBI_ADDR_CMD_FIFO_WRD_1 = 1;
localparam EBI_ADDR_CMD_FIFO_WRD_2 = 2;
localparam EBI_ADDR_CMD_FIFO_WRD_3 = 3;
localparam EBI_ADDR_CMD_FIFO_WRD_4 = 4;

reg [15:0] ebi_captured_data[0:3] = 0;

always @ (posedge clk) begin
	if (rst) begin
		ebi_captured_data <= 0;
		cmd_fifo_wr_en <= 1'b0;
	end else begin
		if (cs & wr) begin
			ebi_captured_data[addr] <= data_in;
			if (addr == EBI_ADDR_CMD_FIFO_WRD_4) begin
				cmd_fifo_wr_en <= 1'b1;
			end else
				cmd_fifo_wr_en <= 1'b0;
			end
		end
	end
end

assign cmd_fifo_data_in = {	ebi_captured_data[EBI_ADDR_CMD_FIFO_WRD_1],
				ebi_captured_data[EBI_ADDR_CMD_FIFO_WRD_2],
				ebi_captured_data[EBI_ADDR_CMD_FIFO_WRD_3],
				ebi_captured_data[EBI_ADDR_CMD_FIFO_WRD_4],
			};

endmodule			
