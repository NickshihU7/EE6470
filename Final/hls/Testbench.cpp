#include "systemc.h"
#include "Testbench.h"
#include "in_real.h"
#include "in_imag.h"
#include <cstdio>
#include <cstdlib>
#define length 16
using namespace std;

Testbench::Testbench(sc_module_name n) : sc_module(n)
{
  SC_THREAD(feed_input);
  sensitive << i_clk.pos();
  dont_initialize();
  SC_THREAD(fetch_result);
  sensitive << i_clk.pos();
  dont_initialize();
}

Testbench::~Testbench() {
    cout << "Total run time = " << total_run_time << endl;
}

void Testbench::feed_input() {

    n_txn = 0;
    max_txn_time = SC_ZERO_TIME;
    min_txn_time = SC_ZERO_TIME;
    total_txn_time = SC_ZERO_TIME;

#ifndef NATIVE_SYSTEMC
    o_real.reset();
    o_imag.reset();
#endif
    o_rst.write(false);
    wait(5);
    o_rst.write(true);
    wait(1);
    total_start_time = sc_time_stamp();
    sc_dt::sc_int<16> buffer_r;
    sc_dt::sc_int<16> buffer_i;   

  while(true)
  { 
    cout << "Input starts" << endl;
    for (int i = 0; i != length; ++i) {
    
        buffer_r = input_r[i];
        buffer_i = input_i[i];
#ifndef NATIVE_SYSTEMC
        o_real.put(buffer_r);
        o_imag.put(buffer_i);
#else
        o_real.write(buffer_r);
        o_imag.write(buffer_i);
#endif
    }
  }
}

void Testbench::fetch_result()
{
#ifndef NATIVE_SYSTEMC
    i_result_real.reset();
    i_result_imag.reset();
#endif
    wait(5);
    wait(1);
    sc_dt::sc_int<16> result_r[16] = {0};
    sc_dt::sc_int<16> result_i[16] = {0};

 while(true)
 { 
    for (int j = 0; j != length; ++j) {
#ifndef NATIVE_SYSTEMC
        result_r[j] = i_result_real.get();
        result_i[j] = i_result_imag.get();
#else
        result_r[j] = i_result_real.read();
        result_i[j] = i_result_imag.read();
#endif
   }
 }
    for (int x = 0; x != length; ++x) {
        cout<<"out_real = "<<result_r[x];
    }
    for (int y = 0; y != length; ++y) {
        cout<<"out_imag = "<<result_i[y];
    }
    total_run_time = sc_time_stamp() - total_start_time;
    sc_stop();
}