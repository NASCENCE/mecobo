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
		output [63:0] 	cmd_fifo_data_in,   // input into cmd FIFO
		output reg	cmd_fifo_wr_en,
		input  		cmd_fifo_almost_full, 
		input  		cmd_fifo_full,
		input  		cmd_fifo_almost_empty,
		input  		cmd_fifo_empty,
		//Interace from SAMPLE DATA fifo
		input 	[15:0] 	sample_fifo_data_out,   //data FROM sample fifo
		output reg	sample_fifo_rd_en,
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

reg [15:0] ebi_captured_data[0:3];

integer i;
always @ (posedge clk) begin
	if (rst) begin
		cmd_fifo_wr_en <= 1'b0;
		for ( i = 1; i < 5; i = i + 1) 
			ebi_captured_data[i] <= 16'h0000;
	end else begin
		if (cs & wr) begin
			ebi_captured_data[addr] <= data_in;
			if (addr == EBI_ADDR_CMD_FIFO_WRD_4) begin
				cmd_fifo_wr_en <= 1'b1;
			end else begin
				cmd_fifo_wr_en <= 1'b0;
			end
		end
	end
end

genvar j;
for (j = 0; j < 4; j = j + 1) begin : blu
	assign cmd_fifo_data_in[(j+1) * 16:(j) * 16] =  ebi_captured_data[i + 1];
end

endmodule			
