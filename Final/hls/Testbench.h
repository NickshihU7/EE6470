#ifndef TESTBENCH_H_
#define TESTBENCH_H_

#include <string>
using namespace std;

#include <systemc>
using namespace sc_core;

#ifndef NATIVE_SYSTEMC
#include <cynw_p2p.h>
#endif

#include "fft_parameter.h"

class Testbench : public sc_module {
public:
  sc_in_clk i_clk;
  sc_out<bool> o_rst;
  sc_in <bool> data_req; 
  sc_out<bool> data_valid;
  sc_in <bool> data_ready; 
  sc_out<bool> data_ack;

#ifndef NATIVE_SYSTEMC
  cynw_p2p< sc_dt::sc_int<16> >::base_out o_real;
  cynw_p2p< sc_dt::sc_int<16> >::base_out o_imag;
  cynw_p2p< sc_dt::sc_int<16> >::base_in i_result_real;
  cynw_p2p< sc_dt::sc_int<16> >::base_in i_result_imag;
#else
  sc_fifo_out< sc_dt::sc_int<16> > o_real;
  sc_fifo_out< sc_dt::sc_int<16> > o_imag;
  sc_fifo_in< sc_dt::sc_int<16> > i_result_real;
  sc_fifo_in< sc_dt::sc_int<16> > i_result_imag;
#endif

  SC_HAS_PROCESS(Testbench);

  Testbench(sc_module_name n);
  ~Testbench();

private:
  
  unsigned int length;
  unsigned int n_txn;
  sc_time max_txn_time;
  sc_time min_txn_time;
  sc_time total_txn_time;
  sc_time total_start_time;
  sc_time total_run_time;

  void feed_input();
  void fetch_result();
};
#endif
