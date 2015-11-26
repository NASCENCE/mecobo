gui_open_window Wave
gui_sg_create main_clocks_group
gui_list_add_group -id Wave.1 {main_clocks_group}
gui_sg_addsignal -group main_clocks_group {main_clocks_tb.test_phase}
gui_set_radix -radix {ascii} -signals {main_clocks_tb.test_phase}
gui_sg_addsignal -group main_clocks_group {{Input_clocks}} -divider
gui_sg_addsignal -group main_clocks_group {main_clocks_tb.CLK_IN1}
gui_sg_addsignal -group main_clocks_group {{Output_clocks}} -divider
gui_sg_addsignal -group main_clocks_group {main_clocks_tb.dut.clk}
gui_list_expand -id Wave.1 main_clocks_tb.dut.clk
gui_sg_addsignal -group main_clocks_group {{Status_control}} -divider
gui_sg_addsignal -group main_clocks_group {main_clocks_tb.RESET}
gui_sg_addsignal -group main_clocks_group {main_clocks_tb.LOCKED}
gui_sg_addsignal -group main_clocks_group {{Counters}} -divider
gui_sg_addsignal -group main_clocks_group {main_clocks_tb.COUNT}
gui_sg_addsignal -group main_clocks_group {main_clocks_tb.dut.counter}
gui_list_expand -id Wave.1 main_clocks_tb.dut.counter
gui_zoom -window Wave.1 -full
