module toplevel_tb;
reg clk, reset;

always begin
  #10 clk = !clk;
end

initial begin
  clk = 0;
  reset = 1;
  #10
  reset = 0;
  #10

end
endmodule
