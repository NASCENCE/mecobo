module xbar_control (
    input ebi_clk, //System clock
    input sclk, //XBAR state clock (but not the clock that goes to the XBAR)
    input reset,
    input cmd_bus_enable,
    input re,
    input cmd_bus_wr,
    input [31:0]cmd_bus_data,
    output reg [15:0] data_out,
    output reg busy,
    input [15:0] cmd_bus_addr,
    //facing the DAC
    output reg xbar_clock, //clock going to xbar
    output reg pclk, //keep low until we're done, single pulse will set transistors in xbar.
output sin);

//Controller select logic. 
parameter POSITION = 0;
wire cs;
assign cs = (cmd_bus_enable & (cmd_bus_addr[15:8] == POSITION));


localparam [7:0] 
ADDR_CMD_REG = 8'h20,
ID_REG   = 8'h09;

//EBIcmd_bus_data capture
reg [31:0] xbar_config_reg [0:15];   //16 * 32 = 512 bits in total to program XBAR
reg [31:0] command = 0;
reg reset_cmd_reg;


integer i;
always @ (posedge ebi_clk) 
begin
    if (reset) begin
        //xbar_config_reg <= 0;
        for (i = 0; i < 16; i = i + 1) begin
            xbar_config_reg[i] <= 0;
        end

    end else begin
        if (reset_cmd_reg) 
            command <= 0;
        else
            if (cs & cmd_bus_wr) begin
                if (cmd_bus_addr[7:0] == ADDR_CMD_REG) begin  
                    command <= cmd_bus_data;
                end else begin
                    xbar_config_reg[cmd_bus_addr[3:0]] <= cmd_bus_data; //offset into ebi big register since we only tx 4 bytes at a time, 2^4 = 16 words of 32 bits
                end
            end

            if (cs & re) begin
                if(cmd_bus_addr[7:0] == ID_REG)
                    data_out <= 16'h7ba2; //kinda looks like 'XbaR'... no?
            end else begin
                data_out <= 0;
            end

    end
end


//Control logic
//------------------------------------------------------------------------------------
reg [8:0] counter = 0;  //clock out 512 pulses.
reg shift_out_cmd_bus_enable;
reg load_shift_reg;
reg count_up;
reg count_res;

parameter init =            5'b00001;
parameter load    =         5'b00010;
parameter pulse_pclk =      5'b00100;
parameter pulse_xbar_clk =  5'b01000;
parameter load_shift =      5'b10000;

reg[4:0] state, nextState;

initial begin
    state <= init;
end

//State transitions.
always @ (posedge sclk) begin
    if (reset) state <= init; 
    else state <= nextState;
end

//Next state function
always @ (*) begin
    nextState = 5'bXXXXX;
    shift_out_cmd_bus_enable = 1'b0;
    count_up = 1'b0;
    count_res = 1'b0;
    xbar_clock = 1'b0;
    pclk = 1'b1;
    load_shift_reg = 1'b0;
    busy = 1'b0;
    reset_cmd_reg = 1'b0;

    case (state)
        //Idle / init, waiting for something to do.
        init: begin
            nextState = init;
            count_res = 1'b1;

            if (command != 0) begin
                nextState = load;
                load_shift_reg = 1'b1;
                busy = 1'b1;
            end
        end

        pulse_xbar_clk: begin
            nextState = load;

            shift_out_cmd_bus_enable = 1'b1;  //shift next bit next flank
            count_up = 1'b1; 
            xbar_clock = 1'b1;  //clock out the current topmost bit.
            busy = 1'b1;

            if (counter == 511) begin
                reset_cmd_reg = 1'b1;
                nextState = pulse_pclk;
            end else if (counter[4:0] == 5'h1F)   //2^5 = 32, which is how many bits are clocked in this round 
                nextState = load_shift;
        end

        load: begin
            busy = 1'b1;
            nextState = pulse_xbar_clk;
        end

        load_shift: begin
            busy = 1'b1;
            load_shift_reg = 1'b1;
            nextState = load;
        end

        pulse_pclk: begin
            busy = 1'b1;
            nextState = init;
            pclk = 1'b0; //give a pulse.
            count_res = 1'b1;
        end
    endcase
end


//Data path, shift register clocked by the slower clock
//------------------------------------------------------------------------------------
reg [31:0] shift_out_register = 0;
always @ (posedge sclk) begin
    if (reset) 
        shift_out_register <= 0;
    else begin
        //Reset counter if asked to.
        if (count_res)
            counter <= 0;
        else 
            if (count_up)
                counter <= counter + 1;

            if (load_shift_reg)
                shift_out_register <= xbar_config_reg[counter[8:5]];  // 2^4 = 16, so we use this index to select which part of big xbar reg to load.
            else
                if (shift_out_cmd_bus_enable)
                    shift_out_register <= {shift_out_register[30:0], 1'b0};
    end
end

assign sin = shift_out_register[31];

endmodule
