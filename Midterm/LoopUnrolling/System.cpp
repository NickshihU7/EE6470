#include "System.h"
System::System( sc_module_name n, string input_bmp, string output_bmp ): sc_module( n ),
	tb("tb"), sobel_filter("sobel_filter"), clk("clk", CLOCK_PERIOD, SC_NS), rst("rst"), _output_bmp(output_bmp)
{
	tb.i_clk(clk);
	tb.o_rst(rst);
	sobel_filter.i_clk(clk);
	sobel_filter.i_rst(rst);
	tb.o_r(o_red);
	tb.o_g(o_green);
	tb.o_b(o_blue);
	tb.i_red(red);
	tb.i_green(green);
	tb.i_blue(blue);
	sobel_filter.i_r(o_red);
	sobel_filter.i_g(o_green);
	sobel_filter.i_b(o_blue);
	sobel_filter.o_red(red);
	sobel_filter.o_green(green);
	sobel_filter.o_blue(blue);

  tb.read_bmp(input_bmp);
}

System::~System() {
  tb.write_bmp(_output_bmp);
}
