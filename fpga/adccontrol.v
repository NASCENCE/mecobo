// Two clock domains:
//  CK1 The controller domain (runs at system clock)
//  CK2 The the ADC domain; which is the serial clock in/out to the chip.
// 
// CK1 is used for access to the EBI interface and is also used for the state 
// machine that updates access_reg every clock cycle.
// CK2 is used for for clocking the DOUT and DIN lines as well as the shift reg that eats DIN
// and DOUT.



module adc_control (
  input clk,   //main system clock (100Mhz?)
  input sclk,  //clocks the serial interface.
  input reset,

  input [18:0] addr,
  input [15:0] data_in,
  output reg [15:0] data_out,

  output reg busy,


  // interfacing the chip.
  output reg cs,
  //data line into the cip.
  output adc_din,
  //clocking the data in line to the chip. Max 10Mhz on this clock. 
  output adc_sclk,
  //serial data from chip
  input adc_dout
);

parameter POSITION = 0;
localparam BASE_ADDR = (POSITION << 8);

localparam [18:0] 
ADDR_ADC_RESET  = BASE_ADDR + 0,
ADDR_ADC_REGISTER = BASE_ADDR + 1,
ADDR_ADC_SAMPLE_CHAN_0 = BASE_ADDR + 2,
ADDR_ADC_SAMPLE_CHAN_1 = BASE_ADDR + 3,
ADDR_ADC_SAMPLE_CHAN_2 = BASE_ADDR + 4,
ADDR_ADC_SAMPLE_CHAN_3 = BASE_ADDR + 5,
ADDR_ADC_SAMPLE_CHAN_4 = BASE_ADDR + 6,
ADDR_ADC_SAMPLE_CHAN_5 = BASE_ADDR + 7,
ADDR_ADC_SAMPLE_CHAN_6 = BASE_ADDR + 8,
ADDR_ADC_SAMPLE_CHAN_7 = BASE_ADDR + 9;


localparam [15:0]
ADC_CMD_RESET = 1,
ADC_CMD_GET_SAMPLES = 2;


always @ (clk) begin
  if (addr == ADDR_ADC_REGISTER)
    //if not shifting out data.
    if (!shift_out)
      adc_register <= data_in;
  else
    data_out <= 16'b0;
end

//8 words, 16 bits memory.
reg [15:0]  adc_register;


//State machine for shifting in data using sclk and 
//copying over to temp register.
reg [15:0] tmp_register [2:0];
reg [15:0] shift_register;
reg [15:0] shift_in_register;

parameter NUM_STATES = 2;
parameter output_reg   = 1'b0;
parameter get_values   = 1'b1;

reg [NUM_STATES - 1:0] state;


reg [3:0] outcounter;
reg res_outcounter;
reg res_outreg;
reg inc_outcounter;
reg shift_out; //shift out to ADC
reg shift_in; //shift into FPGA
reg copy_to_tmp;

always @ (posedge sclk)  begin
  if (reset) begin
    outcounter <= 0;
    adc_register <= 16'h0000;
  end else begin
    if 
      (res_outcounter) outcounter <= 0; 
    else
      if (inc_outcounter) 
        outcounter <= outcounter + 1;

    //Shift register
    if (shift_out) 
      adc_register <= {adc_register[14:0], 1'b0};

    //if (shift_in)
      //Do in shifting.
    //Copy from shift reg
    if (copy_to_tmp) 
      tmp_register[shift_in_register[15:13]] <= shift_in_register;
  end
end

assign adc_din = adc_register[15];

//Control state machine.
always @ (posedge sclk) 
begin
  if (reset) begin
    cs <= 1'b0;
    outcounter <= 4'b0;
    state <= get_values;
    shift_in <= 1'b0;
    shift_out <= 1'b0;
  end
  else begin
    case(state)

      output_reg: begin
        busy <= 1'b1;
        cs <= 1'b1;
        shift_out <= 1'b1;
        shift_in <= 1'b0;
        inc_outcounter <= 1'b1;
        res_outcounter <= 1'b0;
        copy_to_tmp <= 1'b0;
        state <= output_reg;

        if (outcounter == 15) begin
          res_outcounter <= 1'b1;
          state <= get_values; 
        end else begin
          inc_outcounter <= 1'b1;
        end
      end

      //Default state.
      get_values: begin
        busy <= 1'b0;
        cs <= 1'b0;
        shift_out <= 1'b0;
        shift_in <= 1'b1;
        copy_to_tmp <= 1'b0;
        inc_outcounter = 1'b0;
        res_outreg <= 1'b1;
        res_outcounter <= 1'b0;
        state <= get_values;
        if (outcounter == 15) begin
          copy_to_tmp <= 1'b1;
          res_outcounter <= 1'b1;
        end else begin
          inc_outcounter = 1'b1;
        end

        //Check if user has written a command that should be set on the ADC.
        //Else keep copying samples. 
        if (adc_register != 16'h0000)
          state <= output_reg;
      end
    endcase
  end
end

//TODO: statemachine that controls the sample rate copying.
//Controlled by fast clock. 


endmodule
