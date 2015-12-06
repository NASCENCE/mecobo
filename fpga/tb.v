
`timescale 1ns / 10ps

module toplevel_tb;


reg clk,reset;
reg [255:0] sample;
wire q;

wire [15:0] ebi_data_wires;
reg [15:0] ebi_data;

reg [18:0] ebi_addr;
reg ebi_wr;
reg ebi_rd;
reg ebi_cs;

reg [15:0] got;

reg uc_clk = 0;

always begin
  #20 uc_clk = ~uc_clk;
end

integer               data_file    ; // file handler
integer               scan_file    ; // file handler
integer		      oper;
reg   [15:0] file_ebi_data;
reg   [18:0] file_ebi_addr;
reg   [15:0] ebi_captured_data;
`define NULL 0 


initial begin

  $dumpfile("top.vcd");
  $dumpvars(0,mecobo0);

  uc_clk = 0;
  clk = 0;
  reset = 1;
  sample = 1;
  ebi_rd = 1'b0;
  ebi_wr = 1'b0;
  ebi_cs = 1'b0;

  data_file = $fopen("/home/lykkebo/mecobo/fpga/build/instructions.txt", "r");
  if (data_file == `NULL) begin
    $display("data_file handle was NULL");
    $finish;
  end
end

always @(posedge uc_clk) begin
  scan_file = $fscanf(data_file, "%s %x %x", oper, file_ebi_addr, file_ebi_data); 
  if (!$feof(data_file)) begin
    //use captured_data as you would any other wire or reg value;
 
    ebi_rd <= 1'b0;
    ebi_wr <= 1'b0;
    ebi_cs <= 1'b0;
   
    case(oper)
    "s": begin
      $display("RESET set to %x\n", file_ebi_addr);
      reset <= file_ebi_addr;
    end
    "r": begin
      $display("READ from %x", file_ebi_addr);
      ebi_addr <= file_ebi_addr;
      ebi_data <= 16'hZ;
      ebi_captured_data <= ebi_data_wires;
      ebi_rd <= 1'b1;
      ebi_wr <= 1'b0;
      ebi_cs <= 1'b1;
    end 

    "w": begin
      $display("WRITE %x to %x", file_ebi_data, file_ebi_addr);
      ebi_addr <= file_ebi_addr;
      ebi_data <= file_ebi_data;
      ebi_wr <= 1'b1;
      ebi_rd <= 1'b0;
      ebi_cs <= 1'b1;
    end

    "h": begin
      $display("Waiting for %d", file_ebi_addr);
      #(file_ebi_addr);
    end

    "q": begin
      $display("Quit command given.");
      $finish;
    end

     default: begin
      $display("Unknown operation: %d", oper);
      $finish;
      end
    endcase
  end
end


assign ebi_data_wires = ebi_data;

always begin
  #10 clk = ~clk;
end

wire [60:1] HW_tb;
wire [57:1] HN_tb;
mecobo mecobo0 (
.osc(clk),
.reset(reset),
.ebi_data(ebi_data_wires),
.ebi_addr(ebi_addr),
.ebi_wr(~ebi_wr),
.ebi_rd(~ebi_rd),
.ebi_cs(~ebi_cs),
.HN(HN_tb),
.HW(HW_tb)
);

assign HN_tb[1] = HN_tb[2];


endmodule
