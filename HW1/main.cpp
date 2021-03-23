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
  sc_fifo<unsigned char> r;
  sc_fifo<unsigned char> g;
  sc_fifo<unsigned char> b;
  sc_fifo<double> red;
  sc_fifo<double> green;
  sc_fifo<double> blue;
  tb.i_clk(clk);
  tb.o_rst(rst);
  gaussian_blur.i_clk(clk);
  gaussian_blur.i_rst(rst);
  tb.o_r(r);
  tb.o_g(g);
  tb.o_b(b);
  tb.i_red(red);
  tb.i_green(green);
  tb.i_blue(blue);
  gaussian_blur.i_r(r);
  gaussian_blur.i_g(g);
  gaussian_blur.i_b(b);
  gaussian_blur.o_red(red);
  gaussian_blur.o_green(green);
  gaussian_blur.o_blue(blue);

  tb.read_bmp(argv[1]);
  sc_start();
  std::cout << "Simulated time == " << sc_core::sc_time_stamp() << std::endl;
  tb.write_bmp(argv[2]);

  return 0;
}
