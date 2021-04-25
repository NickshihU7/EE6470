#include "System.h"
System::System( sc_module_name n, string input_bmp, string output_bmp ): sc_module( n ),
	tb("tb"), sobel_filter("sobel_filter"), clk("clk", CLOCK_PERIOD, SC_NS), rst("rst"), _output_bmp(output_bmp)
{
	tb.i_clk(clk);
	tb.o_rst(rst);
	sobel_filter.i_clk(clk);
	sobel_filter.i_rst(rst);
	tb.o_rgb(rgb);
	tb.i_red(red);
	tb.i_green(green);
	tb.i_blue(blue);
	sobel_filter.i_rgb(rgb);
	sobel_filter.o_red(red);
	sobel_filter.o_green(green);
	sobel_filter.o_blue(blue);

  tb.read_bmp(input_bmp);
}

System::~System() {
  tb.write_bmp(_output_bmp);
}
