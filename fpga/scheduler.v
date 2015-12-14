module scheduler (	input 			clk,
input 			rst,

//interface to timer.
input 	[31:0]		current_time,
input 			dac_busy,
input			adc_busy,
input			xbar_busy,

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
output 	reg [15:0] 		cmd_bus_addr,
output 	reg [31:0] 		cmd_bus_data,
output	reg		        cmd_bus_en,
output	reg		        cmd_bus_rd,
output	reg		        cmd_bus_wr
);



//wire [2:0] address_h = command[ADDR_H:ADDR_L+13];
//wire address_l        = command[ADDR_L];
//wire mux_address = {address_h,address_l})

wire [1:0] mux_address = command[ADDR_H:ADDR_L+14];
/*
reg addressed_unit_busy;

always @ ( * ) begin
case (mux_address) 
2'b01: addressed_unit_busy = adc_busy;
2'b10: addressed_unit_busy = dac_busy;
2'b11: addressed_unit_busy = xbar_busy;
default: addressed_unit_busy = 1'b0;
endcase
end
*/
wire addressed_unit_busy = ( mux_address[1] | mux_address[0] ) & ( adc_busy | dac_busy | xbar_busy);



/*
wire addressed_unit_busy = (  ((address & 8'b01100000) & adc_busy ) | 
((address & 8'b11110000) & dac_busy ) | 
((address & 8'b11110001) & xbar_busy) );

*/
localparam [5:0] fetch 		= 6'b000001,
fifo_wait	                = 6'b000010,
exec		                = 6'b000100,
idle		                = 6'b001000,
exec_wait   	            = 6'b010000,
unit_busy                   = 6'b100000;

//control section state machine.
reg [5:0] state, nextState;

always @ (posedge clk or posedge rst) begin
    if (rst) state <= fetch;
    else state <= nextState;
end


//output and next state logic.
//note that outputs are not registered.


reg writeCommandReg = 1'b0;
reg resetCommandReg = 1'b0;
reg cmd_bus_load = 1'b0;

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
    nextState = 5'bXXXXX;
    cmd_fifo_rd_en = 1'b0;
    writeCommandReg = 1'b0;
    resetCommandReg = 1'b0;
    cmd_bus_load = 1'b0;
    case (state)
        fetch: begin
            if (cmd_fifo_empty) begin   //idle while waiting for time to tick or if fifo is empty
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

        unit_busy: begin
            if (addressed_unit_busy) 
                nextState = unit_busy;
            else
                nextState = exec;
        end 
        //command reg is written and now 
        exec: begin
            nextState = exec; 
            //Command time 0 is a special sentinent value that allows the item to be executed no matter
            //the current time of the system. It's used for things like setting up recording items, etc.
            //TODO: This creates a horribly long timing path-- the ADDRESS goes into a comparator that
            //if 1 then feeds the greater-than >= comparator. This is evil.
            if ((current_time >= command[TIME_H:TIME_L]) | (command[TIME_H:TIME_L] == 0)) begin
                cmd_bus_load = 1'b1;
                nextState = exec_wait;	
            end
        end

        exec_wait: begin
            nextState = fetch; 
            cmd_bus_load = 1'b1;
            //allow 1 more cycle time to catch. glitch potential?
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
always @ (posedge clk) begin
    if (rst)  begin
        command <= 0;
        cmd_bus_data <= 0;
        cmd_bus_addr <= 0;
    end else begin
        //Command register handling
        if (resetCommandReg) 
            command <= 0;
        else begin
            if (writeCommandReg) begin
                command <= cmd_fifo_dout;
            end
        end

        //Loading the command bus path
        if (cmd_bus_load) begin
            cmd_bus_addr[15:0] <= command[ADDR_H:ADDR_L];
            cmd_bus_data <= command[DATA_H:DATA_L];
            cmd_bus_wr <= 1'b1;
            cmd_bus_en <= 1'b1;
        end else begin
            cmd_bus_addr <= 0;
            cmd_bus_data <= 0;
            cmd_bus_wr <= 1'b0;
            cmd_bus_en <= 1'b0;
        end
    end
end


endmodule
