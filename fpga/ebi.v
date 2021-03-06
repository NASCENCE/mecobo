`define WITH_DB

module ebi(	
input           clk,
input           rst,
output reg      softy_reset,
//External interface to the world
input [15:0]    data_in,
output reg [15:0] data_out,
input [18:0]    addr,
input           rd,
input           wr,
input           cs,
input           global_clock_clk,
output [31:0]   global_clock,
output          global_clock_running,
//Interface to CMD fifo
output [79:0] 	cmd_fifo_data_in,   // input into cmd FIFO
output reg      cmd_fifo_wr_en,
input           cmd_fifo_almost_full, 
input           cmd_fifo_full,
input           cmd_fifo_almost_empty,
input           cmd_fifo_empty,
input	[9:0]	cmd_fifo_data_count,
//Interace from SAMPLE DATA fifo
input 	[15:0] 	sample_fifo_data_out,   //data FROM sample fifo
output reg	    sample_fifo_rd_en,
input 		    sample_fifo_almost_full,
input		    sample_fifo_full,
input 		    sample_fifo_almost_empty,
input		    sample_fifo_empty,
input	[15:0]	sample_fifo_data_count,
//TODO: DAC buffers.
output reg      irq,
//various status lines
input           xbar_busy
);	

// ------------- EBI INTERFACE -----------------

localparam EBI_ADDR_STATUS_REG 		= 0;
localparam EBI_ADDR_CMD_FIFO_WRD_1 	= 1;
localparam EBI_ADDR_CMD_FIFO_WRD_2 	= 2;
localparam EBI_ADDR_CMD_FIFO_WRD_3 	= 3;
localparam EBI_ADDR_CMD_FIFO_WRD_4 	= 4;
localparam EBI_ADDR_CMD_FIFO_WRD_5 	= 5;
localparam EBI_ADDR_NEXT_SAMPLE 	  = 6;
localparam EBI_ADDR_TIME_REG 	      = 7;
localparam EBI_ADDR_READ_TIME_L     = 9;
localparam EBI_ADDR_READ_TIME_H     = 10;
localparam EBI_ADDR_READ_SAMPLE_FIFO_DATA_COUNT = 11;
localparam EBI_ADDR_READBACK = 12;
localparam EBI_ADDR_READ_CMD_FIFO_DATA_COUNT = 13;
localparam EBI_ADDR_DB_PRESENT = 14;



localparam TIME_CMD_RUN   = 'hDEAD;
localparam TIME_CMD_RESET = 'hBEEF;

localparam CMD_SOFT_SYSTEM_RESET = 'hD00D;

reg [15:0] ebi_captured_data[0:10];
reg [15:0] ebi_last_word;

wire rd_transaction_done;
wire wr_transaction_done;

reg [31:0] clock_reg = 0;

//Control state machine
localparam [5:0]	idle 		= 6'b000001,
fetch		            		= 6'b000010,
trans_over	        		= 6'b000100,
fifo_read           			= 6'b001000,
fifo_read_next      			= 6'b010000,
time_cmd            			= 6'b100000;

reg [5:0] state, nextState;

initial begin
    state = fetch;
end

always @ (posedge clk) begin
    if (rst) state <= fetch;
    else state <= nextState;
end

reg capture_fifo_data;
reg run_time;
reg reset_time;


//EBI INTERFACE
always @ ( * ) begin
    nextState = 6'bXXXXXX;
    cmd_fifo_wr_en = 1'b0;

    sample_fifo_rd_en = 1'b0;
    capture_fifo_data = 1'b0;
    run_time = 1'b0;
    reset_time = 1'b0;
    softy_reset = 1'b0;

    case (state)
        fetch: begin
            nextState = fetch;
            if (cs & wr) begin
                if (addr[7:0] == EBI_ADDR_CMD_FIFO_WRD_5)     nextState = trans_over;
                else if (addr[7:0] == EBI_ADDR_TIME_REG)      nextState = time_cmd; // = 1'b1;
            end else if (cs & rd) begin
                if (addr[7:0] == EBI_ADDR_NEXT_SAMPLE)        nextState = fifo_read;
            end
        end

        trans_over: begin
            nextState = trans_over;
            if (wr_transaction_done) begin
                nextState = fetch;
                cmd_fifo_wr_en = 1'b1;
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

        /* Capture new data from the FIFO on the next edge since it's ready now */
        fifo_read_next: begin
            nextState = fetch;
            capture_fifo_data = 1'b1;
        end

        time_cmd: begin
            nextState = time_cmd;
            if (wr_transaction_done)  begin
                nextState = fetch;
                $display("ebi: %x\n", ebi_captured_data[EBI_ADDR_TIME_REG]);
                if (ebi_captured_data[EBI_ADDR_TIME_REG] == TIME_CMD_RESET) begin
                    reset_time = 1'b1;
                end else if (ebi_captured_data[EBI_ADDR_TIME_REG] == TIME_CMD_RUN) begin
                    run_time = 1'b1;
                end else if (ebi_captured_data[EBI_ADDR_TIME_REG] == CMD_SOFT_SYSTEM_RESET) begin
                    softy_reset = 1'b1;
                end
            end
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

        for ( i = 0; i < 11; i = i + 1) 
            ebi_captured_data[i] <= 16'h0000;

    end else begin

        status_register <= 	{	cmd_fifo_almost_full, //15
        cmd_fifo_full, //14
        cmd_fifo_almost_empty, //13
        cmd_fifo_empty,  //12
        sample_fifo_almost_full, //11
        sample_fifo_full,  //10
        sample_fifo_empty,  //9
        sample_fifo_almost_empty,  //8
        xbar_busy,
        7'h00
        };

        irq <= status_register[14] | status_register[10];

        /* Driving data out */
        if (cs & rd) begin
            if (addr[7:0] == EBI_ADDR_STATUS_REG) begin
                data_out <= status_register;
                status_register_old <= status_register;
            end else if (addr[7:0] == EBI_ADDR_NEXT_SAMPLE) begin
                data_out <= fifo_captured_data;
            end else if (addr[7:0] == EBI_ADDR_READ_TIME_L) begin
                data_out <= clock_reg[15:0];
            end else if (addr[7:0] == EBI_ADDR_READ_TIME_H) begin
                data_out <= clock_reg[31:16];
            end else if (addr[7:0] == EBI_ADDR_READ_SAMPLE_FIFO_DATA_COUNT) begin
                data_out <= sample_fifo_data_count;
            end else if (addr[7:0] == EBI_ADDR_READBACK) begin
                data_out <= ebi_last_word;
            end else if (addr[7:0] == EBI_ADDR_READ_CMD_FIFO_DATA_COUNT) begin
                data_out <= {6'h0, cmd_fifo_data_count};
            end else if (addr[7:0] == EBI_ADDR_DB_PRESENT) begin
                `ifdef WITH_DB
                    data_out <= 1;
            `else   
                data_out <= 0;
            `endif
            end else
                data_out <= 0;
        end

        //Write to ebi register banks
        if (cs & wr) begin
            ebi_captured_data[addr[7:0]] <= data_in;
            ebi_last_word <= data_in;
        end

        if (capture_fifo_data)  begin
            fifo_captured_data <= sample_fifo_data_out;
        end


    end
end



assign cmd_fifo_data_in[79:64] = ebi_captured_data[1];
assign cmd_fifo_data_in[63:48] = ebi_captured_data[2];
assign cmd_fifo_data_in[47:32] = ebi_captured_data[3];
assign cmd_fifo_data_in[31:16] = ebi_captured_data[4];
assign cmd_fifo_data_in[15:0] = ebi_captured_data[5];
/*
genvar j;
for (j = 0; j < 5; j = j + 1) begin : blu
assign cmd_fifo_data_in[((j+1) * 16)-1:(j) * 16] =  ebi_captured_data[4-j];
end
*/



//-----------------------------------------------------------------------------------
//rd falling edge detection to see if a transaction has finished


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
// clock controlled by state machine 
//-----------------------------------------------------------------------------------

//run_time is a signal from the global state machine 
//it will be set when the corresponding register is written
//in the EBI interface. 
//I did this to keep register assignment out of the state machine. it's simply a matter of 
//fsm coding style and practice.
reg time_running = 1'b0;
reg time_reset = 1'b0;

always @ (posedge clk) begin
    if (rst|softy_reset) begin
        time_running <= 1'b0;
        time_reset <= 1'b1;
    end else if (reset_time) begin
        time_running <= 1'b0;
        time_reset <= 1'b1;
    end else if (run_time) begin
        time_running <= 1'b1;
        time_reset <= 1'b0;
    end
end		

//this is the up-counting thing.
always @ (posedge global_clock_clk) begin
    if(rst|softy_reset) begin
        clock_reg <= 0; 
    end else begin
        if (time_running)
            clock_reg <= clock_reg + 1;
        else if (time_reset)
            clock_reg <= 0;
        else 
            clock_reg <= 0;
    end
end

assign global_clock_running = time_running & (clock_reg > 0);
assign global_clock = clock_reg;


endmodule			
