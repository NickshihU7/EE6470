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
	i_rgb.clk_rst(i_clk, i_rst);
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
		i_rgb.reset();
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
				sc_dt::sc_uint<24> rgb;
#ifndef NATIVE_SYSTEMC
				{
					HLS_DEFINE_PROTOCOL("input");
					rgb = i_rgb.get();
					wait();
				}
#else
				rgb = i_rgb.read();
#endif
				red += rgb.range(7,0) * filter[v][u];
        green += rgb.range(15,8) * filter[v][u];
        blue += rgb.range(23, 16) * filter[v][u];
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
