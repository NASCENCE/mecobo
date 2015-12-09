all: synth map par bitgen trace

synth:
	mkdir -p xst/projnav.tmp/
	xst -intstyle xflow -ifn "mecobo.xst" -ofn "mecobo.syr" 
map:
	ngdbuild -intstyle ise -sd ../sample_fifo/ -sd ../cmd_fifo/ -dd _ngo -nt timestamp -uc toplevel.ucf  -uc ../main_clocks_mmcm/main_clocks.ucf -uc ../xbar_clock.ucf -p xc6slx45-csg324-2 mecobo.ngc mecobo.ngd  
	map -intstyle ise -p xc6slx45-csg324-2 -w -logic_opt off -ol high -t 1 -xt 0 -register_duplication off -r 4 -global_opt off -mt off -ir off -pr off -lc off -power off -o mecobo_map.ncd mecobo.ngd mecobo.pcf 
par:
	par -w -intstyle ise -ol high -mt off mecobo_map.ncd mecobo.ncd mecobo.pcf 
bitgen:
	bitgen -intstyle ise -f mecobo.ut mecobo.ncd 
trace:
	trce -e 3 mecobo -o mecobo_trace
test:
	fuse -v 2 -incremental -d DEBUG -prj mecobo.prj -o toplevel_tb -L unisims_ver -L unimacro_ver -L xilinxcorelib_ver  work.toplevel_tb work.glbl
	./toplevel_tb -tclbatch tb.tcl
