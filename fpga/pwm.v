// This is verilog!

module pwm (clk, reset, data_in, pwm_out);
  input clk;
  input reset;
  input [255:0] data_in;
  output pwm_out;

  reg pwm_out;

  reg [12:0]   pwm_duty;
  reg [12:0]   pwm_cnt;
  reg [12:0]   pwm_period;

  always @(posedge clk) begin
    if (reset == 1'b1) begin
      pwm_duty <= data_in;
      pwm_cnt <= 12'b0;
      pwm_period <= 255;
      pwm_out <= 1'b1;
    end else begin
      //Check if duty cycle over.
      if (pwm_cnt > pwm_duty) begin
        pwm_out <= 1'b0;
      end else begin
        pwm_out <= 1'b1;
      end
          //Reset counter if period over, load new data.
      if (pwm_cnt == pwm_period) begin
        pwm_cnt <= 1'b0;
        pwm_duty <= data_in;
      end else begin
        pwm_cnt <= pwm_cnt + 1;
      end
    end
  end

endmodule
