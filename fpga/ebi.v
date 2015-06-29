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
		output [79:0] 	cmd_fifo_data_in,   // input into cmd FIFO
		output reg	cmd_fifo_wr_en,
		input  		cmd_fifo_almost_full, 
		input  		cmd_fifo_full,
		input  		cmd_fifo_almost_empty,
		input  		cmd_fifo_empty,
		//Interace from SAMPLE DATA fifo
		input 	[15:0] 	sample_fifo_data_out,   //data FROM sample fifo
		output reg	    sample_fifo_rd_en,
		input 		      sample_fifo_almost_full,
		input		        sample_fifo_full,
		input 		      sample_fifo_almost_empty,
		input		        sample_fifo_empty,
		//TODO: DAC buffers.
		output          reg irq
);	

// ------------- EBI INTERFACE -----------------

localparam EBI_ADDR_STATUS_REG 		  = 0;
localparam EBI_ADDR_CMD_FIFO_WRD_1 	= 1;
localparam EBI_ADDR_CMD_FIFO_WRD_2 	= 2;
localparam EBI_ADDR_CMD_FIFO_WRD_3 	= 3;
localparam EBI_ADDR_CMD_FIFO_WRD_4 	= 4;
localparam EBI_ADDR_CMD_FIFO_WRD_5 	= 5;
localparam EBI_ADDR_NEXT_SAMPLE 	  = 6;

localparam EBI_ADDR_CMD_FIFO_MASK = 18'h5;

reg [15:0] ebi_captured_data[0:5];


//Control state machine
localparam [4:0]	idle 		= 5'b00000,
			fetch		            = 5'b00001,
			fifo_load	          = 5'b00010,
			trans_over	        = 5'b00100,
      fifo_read     = 5'b01000,
      fifo_read_next      = 5'b10000;

reg [4:0] state, nextState;

always @ (posedge clk) begin
	if (rst) state <= idle;
	else state <= nextState;
end

reg load_capture_reg;
reg capture_fifo_data;

always @ ( * ) begin
	nextState = 3'bXXX;
	load_capture_reg = 1'b0;
	cmd_fifo_wr_en = 1'b0;
  
  sample_fifo_rd_en = 1'b0;
  capture_fifo_data = 1'b0;

	case (state)
		idle:
			nextState = fetch;
		fetch: begin
			nextState = fetch;
			if (cs & wr) begin
				load_capture_reg = 1'b1;
				if (addr == EBI_ADDR_CMD_FIFO_WRD_5) nextState = fifo_load;
      end else if (cs & rd) begin
        if (addr == EBI_ADDR_NEXT_SAMPLE) nextState = fifo_read;
      end
		end

		fifo_load: begin
			nextState = trans_over;	
			cmd_fifo_wr_en = 1'b1;	
		end

		trans_over: begin
			nextState = trans_over;
			if (!wr & !rd) begin
				nextState = fetch;
			end
		end

    /* Output data. State holds until read goes low again, indicating
    * that the reader has captured data happily, and we can get some more
    * data from the FIFO for the next transaction */
    fifo_read: begin
      nextState = fifo_read;
      if (rd_transaction_done) begin
        nextState = fifo_read_next;
        sample_fifo_rd_en = 1'b1;
      end
    end

    /* Capture new data from the FIFO on the next edge */
    fifo_read_next: begin
      nextState = fetch;
      capture_fifo_data = 1'b1;
    end

	endcase

end


/*****************************************************
             DATA PATH                            
 ***************************************************/
reg [15:0] status_register = 0;
reg [15:0] status_register_old = 0;
reg [15:0] fifo_captured_data = 16'hDEAD;

integer i;
always @ (posedge clk) begin
  if (rst) begin
    status_register <= 0;
    status_register_old <= 0;
    fifo_captured_data <= 16'hDEAD;

		for ( i = 0; i < 6; i = i + 1) 
			ebi_captured_data[i] <= 16'h0000;
	end else begin
		if (load_capture_reg) begin
			ebi_captured_data[addr-1] <= data_in;
		end

		status_register <= 	{	cmd_fifo_almost_full,
						cmd_fifo_full,
						cmd_fifo_almost_empty,
						cmd_fifo_empty,
						sample_fifo_almost_full,
						sample_fifo_full,
						sample_fifo_empty,
						sample_fifo_almost_empty,
						8'h00
					};

	
		irq <= (status_register != status_register_old);	
    /* Driving data out */
    if (cs & rd) begin
      if (addr == EBI_ADDR_STATUS_REG) begin
			  data_out <= status_register;
			  status_register_old <= status_register;
      end else if (addr == EBI_ADDR_NEXT_SAMPLE) begin
        data_out <= fifo_captured_data;
      end
    end

    if (capture_fifo_data)  begin
      fifo_captured_data <= sample_fifo_data_out;
    end

	end
end



genvar j;
for (j = 0; j < 5; j = j + 1) begin : blu
	assign cmd_fifo_data_in[((j+1) * 16)-1:(j) * 16] =  ebi_captured_data[4-j];
end




//-----------------------------------------------------------------------------------
//rd falling edge detection to see if a transaction has finished

wire rd_transaction_done;
wire wr_transaction_done;

reg rd_d = 0;
reg rd_dd = 0;
reg wr_d = 0;
reg wr_dd = 0;

always @ (posedge clk) begin
  if(rst) begin
    rd_d <= 1'b0;
    wr_d <= 1'b0;
    rd_dd <= 1'b0;
    wr_dd <= 1'b0;
  end else begin
    rd_d <= rd & cs;
    rd_dd <= rd_d;
    wr_d <= wr & cs;
    wr_dd <= wr_d;
  end 
end

//if r_dd is high and rd_d is low, we have seen a falling edge.
assign rd_transaction_done = (~rd_d) & (rd_dd);
assign wr_transaction_done = (~wr_d) & (wr_dd);


//-----------------------------------------------------------------------------------




endmodule			
