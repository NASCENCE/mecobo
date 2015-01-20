all: synth map par bitgen

synth:
	mkdir -p xst/projnav.tmp/
	xst -intstyle xflow -ifn "mecobo.xst" -ofn "mecobo.syr" 
map:
	ngdbuild -intstyle ise -dd _ngo -nt timestamp -uc toplevel.ucf -uc main_clocks.ucf -p xc6slx45-csg324-3 mecobo.ngc mecobo.ngd  
	map -intstyle ise -p xc6slx45-csg324-3 -w -logic_opt off -ol high -t 1 -xt 0 -register_duplication off -r 4 -global_opt off -mt off -ir off -pr off -lc off -power off -o mecobo_map.ncd mecobo.ngd mecobo.pcf 
par:
	par -w -intstyle ise -ol high -mt off mecobo_map.ncd mecobo.ncd mecobo.pcf 
bitgen:
	bitgen -intstyle ise -f mecobo.ut mecobo.ncd 

test:
	fuse -incremental -prj mecobo.prj -o toplevel_tb -L unisims_ver -L unimacro_ver -L xilinxcorelib_ver  work.toplevel_tb work.glbl
	./toplevel_tb -tclbatch tb.tcl
