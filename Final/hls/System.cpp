#include "System.h"
System::System( sc_module_name n): sc_module( n ), 
	tb("tb"), one_d_fft("one_d_fft"), clk("clk", CLOCK_PERIOD, SC_NS), rst("rst")
{
	tb.i_clk(clk);
	tb.o_rst(rst);
	one_d_fft.i_clk(clk);
	one_d_fft.i_rst(rst);
	tb.o_real(in_real);
	tb.o_imag(in_imag);
	tb.i_result_real(out_real);
	tb.i_result_imag(out_imag);
	one_d_fft.i_real(in_real);
	one_d_fft.i_imag(in_imag);
	one_d_fft.o_real(out_real);
	one_d_fft.o_imag(out_imag);
}

System::~System() {}

