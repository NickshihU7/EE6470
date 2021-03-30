#ifndef GAUSSIAN_FILTER_H_
#define GAUSSIAN_FILTER_H_
#include <systemc>
using namespace sc_core;

#include "tlm"
#include "tlm_utils/simple_target_socket.h"

#include "filter_def.h"

const double sigma = 3.0;
class GaussianBlur : public sc_module {
public:
  tlm_utils::simple_target_socket<GaussianBlur> t_skt;

  sc_fifo<unsigned char> i_r;
  sc_fifo<unsigned char> i_g;
  sc_fifo<unsigned char> i_b;
  sc_fifo<int> o_red;
  sc_fifo<int> o_green;
  sc_fifo<int> o_blue;

  SC_HAS_PROCESS(GaussianBlur);
  GaussianBlur(sc_module_name n);
  ~GaussianBlur() = default;

private:
  void do_filter();
  double red;
  double green;
  double blue;
  unsigned int base_offset;
  void blocking_transport(tlm::tlm_generic_payload &payload,
                          sc_core::sc_time &delay);
};
#endif
