#ifndef SOBEL_FILTER_H_
#define SOBEL_FILTER_H_
#include <systemc>
using namespace sc_core;

#ifndef NATIVE_SYSTEMC
#include <cynw_p2p.h>
#endif

#include "fft_parameter.h"

class oneD_FFT: public sc_module
{
public:
	sc_in_clk i_clk;
	sc_in < bool >  i_rst;
	sc_in<bool> data_valid;                       
 	sc_in<bool> data_ack;
 	sc_out<bool> data_req;
  	sc_out<bool> data_ready; 
#ifndef NATIVE_SYSTEMC
	cynw_p2p< sc_dt::sc_int<16> >::out o_real;
  	cynw_p2p< sc_dt::sc_int<16> >::out o_imag;
  	cynw_p2p< sc_dt::sc_int<16> >::in i_real;
  	cynw_p2p< sc_dt::sc_int<16> >::in i_imag;
#else
	sc_fifo_out< sc_dt::sc_int<16> > o_real;
  	sc_fifo_out< sc_dt::sc_int<16> > o_imag;
  	sc_fifo_in< sc_dt::sc_int<16> > i_real;
  	sc_fifo_in< sc_dt::sc_int<16> > i_imag;
#endif

	SC_HAS_PROCESS( oneD_FFT );
	oneD_FFT( sc_module_name n );
	~oneD_FFT();
private:
	void do_fft();
	sc_int<16> real[N];
  	sc_int<16> imag[N];
  	sc_int<16> W_real[W];
  	sc_int<16> W_imag[W];
};
#endif
