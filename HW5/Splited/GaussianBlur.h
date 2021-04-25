#ifndef GAUSSIAN_BLUR_H_
#define GAUSSIAN_BLUR_H_
#include <systemc>
using namespace sc_core;

#ifndef NATIVE_SYSTEMC
#include <cynw_p2p.h>
#endif

#include "filter_def.h"

class GaussianBlur: public sc_module
{
public:
	sc_in_clk i_clk;
	sc_in < bool >  i_rst;
#ifndef NATIVE_SYSTEMC
	cynw_p2p< sc_dt::sc_uint<8> >::in i_r;
	cynw_p2p< sc_dt::sc_uint<8> >::in i_g;
	cynw_p2p< sc_dt::sc_uint<8> >::in i_b;
	cynw_p2p< sc_dt::sc_uint<32> >::out o_red;
	cynw_p2p< sc_dt::sc_uint<32> >::out o_green;
	cynw_p2p< sc_dt::sc_uint<32> >::out o_blue;
#else
	sc_fifo_in< sc_dt::sc_uint<8> > i_r;
	sc_fifo_in< sc_dt::sc_uint<8> > i_g;
	sc_fifo_in< sc_dt::sc_uint<8> > i_b;
	sc_fifo_out< sc_dt::sc_uint<32> > o_red;
	sc_fifo_out< sc_dt::sc_uint<32> > o_green;
	sc_fifo_out< sc_dt::sc_uint<32> > o_blue;
#endif

	SC_HAS_PROCESS( GaussianBlur );
	GaussianBlur( sc_module_name n );
	~GaussianBlur();
private:
	void do_filter();
	double red;
  double green;
  double blue;
};
#endif
