#include <cmath>
#include <iomanip>

#include "GaussianBlur.h"
using namespace std;

GaussianBlur::GaussianBlur(sc_module_name n)
    : sc_module(n), t_skt("t_skt"), base_offset(0) {
  SC_THREAD(do_filter);

  t_skt.register_b_transport(this, &GaussianBlur::blocking_transport);
}

void GaussianBlur::do_filter() {
  { wait(CLOCK_PERIOD, SC_NS); }
  int i, j, k, l;
  double filter[filterHeight][filterWidth] = {0,0,0,0,0,0,0,0,0};
  double sum=0.0;

  for (i=0 ; i<filterHeight ; i++) {
      for (j=0 ; j<filterWidth ; j++) {
          k = i - floor(filterHeight/2);
          l = j - floor(filterWidth/2);
          filter[i][j] = exp(-(k*k+l*l)/(2*sigma*sigma))/(2*M_PI*sigma*sigma);
          sum += filter[i][j];
      }
  }

  for (i=0 ; i<filterHeight ; i++) {
      for (j=0 ; j<filterWidth ; j++) {
          filter[i][j] /= sum;
          printf("%f\n",filter[i][j]);
      }
  }

  while (true) {
    red = 0;
    green = 0;
    blue = 0;
    for (unsigned int v = 0; v < filterHeight; ++v) {
      for (unsigned int u = 0; u < filterWidth; ++u) {
        red += i_r.read() * filter[v][u];
        green += i_g.read() * filter[v][u];
        blue += i_b.read() * filter[v][u];
        wait(CLOCK_PERIOD, SC_NS);
      }
    }
    o_red.write(red);
    o_green.write(green);
    o_blue.write(blue);
  }
}

void GaussianBlur::blocking_transport(tlm::tlm_generic_payload &payload,
                                     sc_core::sc_time &delay) {
  sc_dt::uint64 addr = payload.get_address();
  addr = addr - base_offset;
  unsigned char *mask_ptr = payload.get_byte_enable_ptr();
  unsigned char *data_ptr = payload.get_data_ptr();
  word buffer;
  switch (payload.get_command()) {
  case tlm::TLM_READ_COMMAND:
    switch (addr) {
    case GAUSSIAN_FILTER_RESULT_ADDR1:
      buffer.uint = o_red.read();
      break;
    case GAUSSIAN_FILTER_RESULT_ADDR2:
      buffer.uint = o_green.read();
      break;
    case GAUSSIAN_FILTER_RESULT_ADDR3:
      buffer.uint = o_blue.read();
      break;
    default:
      std::cerr << "Error! GaussianBlur::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    data_ptr[0] = buffer.uc[0];
    data_ptr[1] = buffer.uc[1];
    data_ptr[2] = buffer.uc[2];
    data_ptr[3] = buffer.uc[3];
    break;

  case tlm::TLM_WRITE_COMMAND:
    switch (addr) {
    case GAUSSIAN_FILTER_R_ADDR:
      if (mask_ptr[0] == 0xff) {
        i_r.write(data_ptr[0]);
      }
      if (mask_ptr[1] == 0xff) {
        i_g.write(data_ptr[1]);
      }
      if (mask_ptr[2] == 0xff) {
        i_b.write(data_ptr[2]);
      }
      break;
    default:
      std::cerr << "Error! GaussianBlur::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    break;

  case tlm::TLM_IGNORE_COMMAND:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  default:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  }
  payload.set_response_status(tlm::TLM_OK_RESPONSE); // Always OK
}
