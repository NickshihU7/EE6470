#define SOBEL_FILTER_H_
#define SOBEL_FILTER_H_
#include <systemc>
#include <cmath>
#include <iomanip>
using namespace sc_core;

#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "filter_def.h"

struct GaussianBlur0 : public sc_module {
  tlm_utils::simple_target_socket<GaussianBlur0> tsock;

  sc_fifo<unsigned char> i_r;
  sc_fifo<unsigned char> i_g;
  sc_fifo<unsigned char> i_b;
  sc_fifo<int> o_red;
  sc_fifo<int> o_green;
  sc_fifo<int> o_blue;

  SC_HAS_PROCESS(GaussianBlur0);

  GaussianBlur0(sc_module_name n): 
    sc_module(n), 
    tsock("t_skt"), 
    base_offset(0) 
  {
    tsock.register_b_transport(this, &GaussianBlur0::blocking_transport);
    SC_THREAD(do_filter);
  }

  ~GaussianBlur0() {
	}

  unsigned int base_offset;

  void do_filter(){
    { wait(CLOCK_PERIOD, SC_NS); }
    while (true) {
      double red = 0;
      double green = 0;
      double blue = 0;
      for (unsigned int v = 0; v < MASK_Y; ++v) {
        for (unsigned int u = 0; u < MASK_X; ++u) {
          red += i_r.read() * mask1[v][u];
          green += i_g.read() * mask1[v][u];
          blue += i_b.read() * mask1[v][u];
          wait(CLOCK_PERIOD, SC_NS);
        }
      }
      o_red.write(red);
      o_green.write(green);
      o_blue.write(blue);
    }
  }

  void blocking_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay){
    wait(delay);
    // unsigned char *mask_ptr = payload.get_byte_enable_ptr();
    // auto len = payload.get_data_length();
    tlm::tlm_command cmd = payload.get_command();
    sc_dt::uint64 addr = payload.get_address();
    unsigned char *data_ptr = payload.get_data_ptr();

    addr -= base_offset;


    // cout << (int)data_ptr[0] << endl;
    // cout << (int)data_ptr[1] << endl;
    // cout << (int)data_ptr[2] << endl;
    word buffer;

    switch (cmd) {
      case tlm::TLM_READ_COMMAND:
        // cout << "READ" << endl;
        switch (addr) {
          case SOBEL_FILTER_RESULT_ADDR1:
            buffer.uint = o_red.read();
            break;
          case SOBEL_FILTER_RESULT_ADDR2:
            buffer.uint = o_green.read();
            break;
          case SOBEL_FILTER_RESULT_ADDR3:
            buffer.uint = o_blue.read();
            break;
            default:
            std::cerr << "READ Error! GaussianBlur0::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
          }
        data_ptr[0] = buffer.uc[0];
        data_ptr[1] = buffer.uc[1];
        data_ptr[2] = buffer.uc[2];
        data_ptr[3] = buffer.uc[3];
        break;
      case tlm::TLM_WRITE_COMMAND:
        // cout << "WRITE" << endl;
        switch (addr) {
          case SOBEL_FILTER_R_ADDR:
            i_r.write(data_ptr[0]);
            i_g.write(data_ptr[1]);
            i_b.write(data_ptr[2]);
            break;
          default:
            std::cerr << "WRITE Error! GaussianBlur0::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
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
};
//#endif
