module pincontrol ( clk,
reset,
enable,
addr,
data_wr,
data_rd,
data_in,
data_out,
pin,
output_sample,
channel_select,
sample_data,
current_time, 
global_clock_running,
busy
);



input clk;
input reset;
input enable;
input [15:0] addr;
input   [31:0] data_in;
//(* tristate2logic = "yes" *)
output reg [15:0] data_out;
inout pin;

input data_wr;
input data_rd;

input [31:0] current_time;
input global_clock_running;

input output_sample;
input [7:0] channel_select;
output reg [31:0] sample_data;
output reg busy;


parameter [14:0] POSITION = 0;

reg sample_register = 0;
reg [15:0] sample_cnt = 16'h0000;
reg [31:0] nco_counter = 32'h00000000;
reg [31:0] nco_pa = 0;
reg [31:0] end_time = 0;
reg [31:0] rec_start_time = 0;


wire pin_input;
wire enable_in = (enable & (addr[15:8] == POSITION[7:0]));
/*Input, output: PWM, SGEN, CONST */



/*These are byte addresses. */
localparam
ADDR_GLOBAL_CMD = 0,
ADDR_NCO_COUNTER = 1,
ADDR_END_TIME = 2,
ADDR_LOCAL_CMD =  3,
ADDR_SAMPLE_RATE =  4,
ADDR_SAMPLE_REG =  5,
ADDR_REC_START_TIME = 6,
ADDR_SAMPLE_CNT =  7,
ADDR_STATUS_REG =  8,
ADDR_LAST_DATA = 9;


localparam
CMD_CONST = 2,
CMD_SQUARE_WAVE = 3,
CMD_INPUT_STREAM = 4,
CMD_RESET = 5;

reg [31:0] command = 0;
reg [31:0] sample_rate = 0;
reg [31:0] cnt_sample_rate = 0;


/*outputs from state machine */
reg res_sample_counter = 0;
reg dec_sample_counter = 0;
reg update_data_out    = 0;
reg update_sample_cnt = 0;
reg enable_pin_output = 0;
reg const_output_null = 0;
reg const_output_one = 0;

localparam [3:0] 
idle =            4'b0001,
const =           4'b0010,
input_stream =    4'b0100,
enable_out =      4'b1000;

reg [3:0] state, nextState;


/* Command bus data output */
always @ (posedge clk) begin
    if(reset) begin
        sample_data <= 0;
        data_out <= 0;
    end else begin

        if (enable_in & data_rd) begin
            if (addr[7:0] == ADDR_SAMPLE_REG) 
                data_out <= {15'b0, sample_register};
            else if (addr[7:0] == ADDR_SAMPLE_CNT) 
                data_out <= sample_cnt;
            else if (addr[7:0] == ADDR_STATUS_REG)
                data_out <= POSITION;
            else
                data_out <= 16'b0;
        end else
            data_out <= 16'b0;

        if (output_sample & (channel_select == POSITION)) 
            sample_data <= {sample_cnt, POSITION, sample_register};
        else 
            sample_data <= 0;

    end
end


/* Drive output pin from MSB of nco_pa statemachine if output is enabled. */
assign pin = (enable_pin_output) ? nco_pa[31] : 1'bZ; /*Z or 0 */
assign pin_input = pin;


/* ------------------ CAPTURE FROM COMMAND BUS ---------*/

reg reset_cmd = 1'b0;
reg reset_rec_time_register = 1'b0;
reg reset_sample_registers = 1'b0;

always @ (posedge clk) begin
    if (reset) begin
        nco_counter <= 0;
        sample_rate <= 0;
        end_time <= 0;
        rec_start_time <= 0;
    end else begin
        /* This is special handling of the command register and it's kinda wonky.
        *  Problems can happen if two consequtive writes happen close in time 
        *  so that the state machine is transitioning from an execute state
        *  to idle state while a new transaction comes in.
        *
        *  The resolution is to guarantee a hold time of 1 cycle for the 
        *  command bus so that the state machine has time to reset the 
        *  command register AND realize that there is new data available.
        */
        if (reset_cmd)
            command <= 0;
        else if (enable_in & data_wr) begin
            if (addr[7:0] == ADDR_LOCAL_CMD)
                command <= data_in;   //always fetch this.
            else if (addr[7:0] == ADDR_SAMPLE_RATE)
                sample_rate[31:0] <= data_in;
            else if (addr[7:0] == ADDR_NCO_COUNTER)
                nco_counter[31:0] <= data_in;
            else if (addr[7:0] == ADDR_END_TIME)
                end_time[31:0] <= data_in;
        end

        if (reset_rec_time_register)
            rec_start_time <= 0;
        else if (enable_in & data_wr) begin
            if (addr[7:0] == ADDR_REC_START_TIME)
                rec_start_time[31:0] <= data_in;
        end

    end
end


//Say time start is 2, end time is 4. This is a period
//of 4 - 2 = 2. 
//Since we test for end time MORE than end time, end condition will happen at 5.
//but start condition happens at 3. 5-3 = 2. So we have the same period and
//we're good.
wire end_condition = current_time > end_time;
wire start_condition = rec_start_time < current_time;


/* CONTROL LOGIC STATE MACHINE */

always @ (posedge clk) begin
    if (reset) state <= idle;
    else state <= nextState;
end

always @ (*) begin

    nextState = 4'bXXXX;
    enable_pin_output = 1'b0;
    dec_sample_counter = 1'b0;
    res_sample_counter = 1'b0;
    update_data_out = 1'b0;
    const_output_null = 1'b0;
    const_output_one = 1'b0;
    reset_cmd = 1'b0;
    reset_rec_time_register = 1'b0;
    reset_sample_registers = 1'b0;
    busy = 1'b0;

    case (state)
        idle: begin
            nextState = idle;
            res_sample_counter = 1'b1;

            //We don't start doing stuff if the time has not started.
            if (~global_clock_running) begin
                nextState = idle;
            end 
            else if (rec_start_time != 0) begin
                busy = 1'b1;
                reset_cmd = 1'b1;
                nextState = input_stream;
            end else if (command == CMD_INPUT_STREAM) begin
                busy = 1'b1;
                reset_cmd = 1'b1; //we got a new command, so reset the register for more stuff to come!
                update_data_out = 1'b1; 
                res_sample_counter = 1'b1;
                nextState = input_stream;
            end else if (command == CMD_SQUARE_WAVE) begin
                busy = 1'b1;
                reset_cmd = 1'b1; //we got a new command, so reset the register for more stuff to come!
                nextState = enable_out;
            end else if (command == CMD_CONST) begin
                busy = 1'b1;
                reset_cmd = 1'b1; //we got a new command, so reset the register for more stuff to come!
                enable_pin_output = 1'b1;
                const_output_one = 1'b1;
                nextState = const;
            end else if (command == CMD_RESET) begin
                busy = 1'b1;
                reset_cmd = 1'b1;
                nextState = idle;
            end 
        end //end idle state

        enable_out: begin
            enable_pin_output = 1'b1;
            nextState = enable_out;

            if (command == CMD_RESET) begin
                reset_cmd = 1'b1; //we got a new command, so reset the register for more stuff to come!
                nextState = idle;
            end else if ((end_time != 0) & (end_condition)) begin
                reset_cmd = 1'b1; //we are done. no more output.
                const_output_null = 1'b1;
                nextState = idle;
            end
        end

        const: begin
            enable_pin_output = 1'b1;
            const_output_one = 1'b1;

            nextState = const;

            if (command == CMD_RESET) begin
                reset_cmd = 1'b1; //we got a new command, so reset the register for more stuff to come!
                nextState = idle;
            end else if ((end_time != 0) & end_condition) begin
                reset_cmd = 1'b1; //command is finished to we can probably get a new one now.
                const_output_null = 1'b1;
                nextState = idle;
            end
        end

        /*Stream back data. */
        input_stream: begin

            nextState = input_stream;
            if(start_condition & !end_condition) begin 
                if (cnt_sample_rate == 0) begin
                    update_data_out = 1'b1; 
                    res_sample_counter = 1'b1;
                end else begin
                    update_data_out = 1'b0; 
                    dec_sample_counter = 1'b1;
                end
            end

            /*We're streaming input back
            at a certain rate, and will never leave this state
            unless reset is called.
            */
            if (command == CMD_RESET) begin
                reset_cmd = 1'b1; //we got a new command, so reset the register for more stuff to come!
                reset_rec_time_register = 1'b1;
                reset_sample_registers = 1'b1;
                nextState = idle;
            end else if ((end_time != 0) & end_condition) begin
                reset_rec_time_register = 1'b1; //set some registers to zero as well.
                reset_sample_registers = 1'b1;
                nextState = idle;
            end
        end 
    endcase
end

/* STATE MACHINE END */



/* COUNTER REGISTERS CONTROLLED BY STATE MACHINE */
always @ (posedge clk) begin

    if(reset) begin
        sample_cnt <= 0;
        sample_register <= 0;
        cnt_sample_rate <= 0;
    end else begin
        if (reset_sample_registers)  begin
            sample_cnt <= 0;
            sample_register <= 0;
            cnt_sample_rate <= 0;
        end else begin
            if (global_clock_running) begin
                if (res_sample_counter) 
                    cnt_sample_rate <= sample_rate;
                else if (dec_sample_counter) 
                    cnt_sample_rate <= (cnt_sample_rate - 1);

                if (update_data_out)  begin
                    sample_register <= pin_input;
                    sample_cnt <= (sample_cnt + 1);
                end
            end
        end
    end
end


/* ------------------------- NCO ----------------------------------
Output frequency will be roughly freq(clk) * (nco_counter/2^32)
*/
always @ (posedge clk) begin
    if(reset)
        nco_pa <= 0;
    else begin
        if (const_output_null)
            nco_pa <= 0;
        else if (const_output_one) 
            nco_pa <= 32'hFFFFFFFF;
        else
            nco_pa <= nco_pa + nco_counter;
    end
end

endmodule
