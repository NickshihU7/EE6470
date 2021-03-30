#include <iostream>
#include <string>
using namespace std;

// Wall Clock Time Measurement
#include <sys/time.h>

#include "GaussianBlur.h"
#include "Testbench.h"

// TIMEVAL STRUCT IS Defined ctime
// use start_time and end_time variables to capture
// start of simulation and end of simulation
struct timeval start_time, end_time;

// int main(int argc, char *argv[])
int sc_main(int argc, char **argv) {
  if ((argc < 3) || (argc > 4)) {
    cout << "No arguments for the executable : " << argv[0] << endl;
    cout << "Usage : >" << argv[0] << " lena.bmp lena_gaussian.bmp"
         << endl;
    return 0;
  }
  Testbench tb("tb");
  GaussianBlur gaussian_blur("gaussian_blur");
  sc_clock clk("clk", CLOCK_PERIOD, SC_NS);
  sc_signal<bool> rst("rst");
  sc_fifo<unsigned char> r[3];
  sc_fifo<unsigned char> g[3];
  sc_fifo<unsigned char> b[3];
  sc_fifo<double> red;
  sc_fifo<double> green;
  sc_fifo<double> blue;
  tb.i_clk(clk);
  tb.o_rst(rst);
  gaussian_blur.i_clk(clk);
  gaussian_blur.i_rst(rst);
  tb.o_r[0](r[0]);
  tb.o_g[0](g[0]);
  tb.o_b[0](b[0]);
  tb.o_r[1](r[1]);
  tb.o_g[1](g[1]);
  tb.o_b[1](b[1]);
  tb.o_r[2](r[2]);
  tb.o_g[2](g[2]);
  tb.o_b[2](b[2]);
  tb.i_red(red);
  tb.i_green(green);
  tb.i_blue(blue);
  gaussian_blur.i_r[0](r[0]);
  gaussian_blur.i_g[0](g[0]);
  gaussian_blur.i_b[0](b[0]);
  gaussian_blur.i_r[1](r[1]);
  gaussian_blur.i_g[1](g[1]);
  gaussian_blur.i_b[1](b[1]);
  gaussian_blur.i_r[2](r[2]);
  gaussian_blur.i_g[2](g[2]);
  gaussian_blur.i_b[2](b[2]);
  gaussian_blur.o_red(red);
  gaussian_blur.o_green(green);
  gaussian_blur.o_blue(blue);

  tb.read_bmp(argv[1]);
  sc_start();
  std::cout << "Simulated time == " << sc_core::sc_time_stamp() << std::endl;
  tb.write_bmp(argv[2]);

  return 0;
}
