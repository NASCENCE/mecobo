module ebi(	input clk,
		input rst,
		//External interface to the world
		input [15:0] data_in,
		output reg [15:0] data_out,
		input [18:0] addr,
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
		input		sample_fifo_empty,
		//TODO: DAC buffers.
		output          irq
);	

// ------------- EBI INTERFACE -----------------

localparam EBI_ADDR_STATUS_REG 		= 0;
localparam EBI_ADDR_CMD_FIFO_WRD_1 	= 1;
localparam EBI_ADDR_CMD_FIFO_WRD_2 	= 2;
localparam EBI_ADDR_CMD_FIFO_WRD_3 	= 3;
localparam EBI_ADDR_CMD_FIFO_WRD_4 	= 4;

localparam EBI_ADDR_CMD_FIFO_MASK = 18'h5;

reg [15:0] ebi_captured_data[0:3];


//Control state machine
localparam [2:0]	idle 		= 3'b000,
			fetch		= 3'b001,
			fifo_load	= 3'b010,
			trans_over	= 3'b100;

reg [2:0] state, nextState;

always @ (posedge clk) begin
	if (rst) state <= idle;
	else state <= nextState;
end

reg load_capture_reg;

always @ ( * ) begin
	nextState = 3'bXXX;
	load_capture_reg = 1'b0;
	cmd_fifo_wr_en <= 1'b0;
	case (state)
		idle:
			nextState = fetch;
		fetch: begin
			nextState = fetch;
			if (cs & wr) begin
				load_capture_reg = 1'b1;
				if (addr == EBI_ADDR_CMD_FIFO_WRD_4) nextState = fifo_load;
			end
		end

		fifo_load: begin
			nextState = trans_over;	
			cmd_fifo_wr_en <= 1'b1;	
		end

		trans_over: begin
			nextState = trans_over;
			if (!wr & !rd) begin
				nextState = fetch;
			end
		end
	
	endcase

end

//datapath
//
//

assign irq = 				cmd_fifo_almost_full |
					cmd_fifo_full |
					cmd_fifo_almost_empty |
					cmd_fifo_empty |
					sample_fifo_almost_full |
					sample_fifo_full |
					sample_fifo_empty |
					sample_fifo_almost_empty;


integer i;
always @ (posedge clk) begin
	if (rst) begin
		for ( i = 0; i < 4; i = i + 1) 
			ebi_captured_data[i] <= 16'h0000;
	end
	if (load_capture_reg) begin
		ebi_captured_data[addr-1] <= data_in;
	end

	//Reading
	if (addr == EBI_ADDR_STATUS_REG) begin
		data_out <= 	{
					cmd_fifo_almost_full,
					cmd_fifo_full,
					cmd_fifo_almost_empty,
					cmd_fifo_empty,
					sample_fifo_almost_full,
					sample_fifo_full,
					sample_fifo_empty,
					sample_fifo_almost_empty,
					8'h00

				};
	end
end



genvar j;
for (j = 0; j < 4; j = j + 1) begin : blu
	assign cmd_fifo_data_in[((j+1) * 16)-1:(j) * 16] =  ebi_captured_data[3-j];
end

endmodule			
