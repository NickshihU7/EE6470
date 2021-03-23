#ifndef GAUSSIAN_BLUR_H_
#define GAUSSIAN_BLUR_H_
#include <systemc>
using namespace sc_core;

#include "filter_def.h"

const double sigma = 3.0;

class GaussianBlur : public sc_module {
public:
  sc_in_clk i_clk;
  sc_in<bool> i_rst;
  sc_fifo_in<unsigned char> i_r;
  sc_fifo_in<unsigned char> i_g;
  sc_fifo_in<unsigned char> i_b;
  sc_fifo_out<double> o_red;
  sc_fifo_out<double> o_green;
  sc_fifo_out<double> o_blue;

  SC_HAS_PROCESS(GaussianBlur);
  GaussianBlur(sc_module_name n);
  ~GaussianBlur() = default;

private:
  void do_filter();
  double red;
  double green;
  double blue;
};
#endif
