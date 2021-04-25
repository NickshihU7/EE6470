#include <cmath>
#ifndef NATIVE_SYSTEMC
#include "stratus_hls.h"
#endif

#include "GaussianBlur.h"

GaussianBlur::GaussianBlur( sc_module_name n ): sc_module( n )
{
//#ifndef NATIVE_SYSTEMC
	//HLS_FLATTEN_ARRAY(val);
//#endif
	SC_THREAD( do_filter );
	sensitive << i_clk.pos();
	dont_initialize();
	reset_signal_is(i_rst, false);

#ifndef NATIVE_SYSTEMC
	i_r.clk_rst(i_clk, i_rst);
	i_g.clk_rst(i_clk, i_rst);
	i_b.clk_rst(i_clk, i_rst);
  o_red.clk_rst(i_clk, i_rst);
	o_green.clk_rst(i_clk, i_rst);
	o_blue.clk_rst(i_clk, i_rst);
#endif
}

GaussianBlur::~GaussianBlur() {}

// sobel mask
double filter[filterHeight][filterWidth] =
{
  0.077847, 0.123317, 0.077847,
  0.123317, 0.195346, 0.123317,
  0.077847, 0.123317, 0.077847,
};

void GaussianBlur::do_filter() {
	{
#ifndef NATIVE_SYSTEMC
		HLS_DEFINE_PROTOCOL("main_reset");
		i_r.reset();
		i_g.reset();
		i_b.reset();
		o_red.reset();
		o_green.reset();
		o_blue.reset();
#endif
		wait();
	}
	while (true) {
		red = 0;
    green = 0;
    blue = 0;
		for (unsigned int v = 0; v<filterHeight; ++v) {
			for (unsigned int u = 0; u<filterWidth; ++u) {
				sc_dt::sc_uint<8> red_read;
				sc_dt::sc_uint<8> green_read;
				sc_dt::sc_uint<8> blue_read;
#ifndef NATIVE_SYSTEMC
				{
					HLS_DEFINE_PROTOCOL("input");
					red_read = i_r.get();
					green_read = i_g.get();
					blue_read = i_b.get();
					wait();
				}
#else
				red_read = i_r.read();
				green_read = i_g.read();
				blue_read = i_b.read();
#endif
				red += red_read * filter[v][u];
        green += green_read * filter[v][u];
        blue += blue_read * filter[v][u];
			}
		}
#ifndef NATIVE_SYSTEMC
		{
			HLS_DEFINE_PROTOCOL("output");
			o_red.put(red);
			o_green.put(green);
			o_blue.put(blue);
			wait();
		}
#else
o_red.write(red);
o_green.write(green);
o_blue.write(blue);
#endif
	}
}
