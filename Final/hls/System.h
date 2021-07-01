#ifndef SYSTEM_H_
#define SYSTEM_H_
#include <systemc>
using namespace sc_core;

#include "Testbench.h"
#ifndef NATIVE_SYSTEMC
#include "oneD_FFT_wrap.h"
#else
#include "oneD_FFT.h"
#endif

class System: public sc_module
{
public:
	SC_HAS_PROCESS( System );
	System( sc_module_name n);
	~System();
private:
  Testbench tb;
#ifndef NATIVE_SYSTEMC
	oneD_FFT_wrapper one_d_fft;
#else
	oneD_FFT one_d_fft;
#endif
	sc_clock clk;
	sc_signal<bool> rst;
#ifndef NATIVE_SYSTEMC
	cynw_p2p< sc_dt::sc_int<16> > in_real;
	cynw_p2p< sc_dt::sc_int<16> > out_real;
	cynw_p2p< sc_dt::sc_int<16> > in_imag;
	cynw_p2p< sc_dt::sc_int<16> > out_imag;
#else
	sc_fifo< sc_dt::sc_int<16> > in_real;
	sc_fifo< sc_dt::sc_int<16> > out_real;
	sc_fifo< sc_dt::sc_int<16> > in_imag;
	sc_fifo< sc_dt::sc_int<16> > out_imag;
#endif

	std::string _output_bmp;
};
#endif
