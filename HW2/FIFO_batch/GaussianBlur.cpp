#include <cmath>

#include "GaussianBlur.h"

GaussianBlur::GaussianBlur(sc_module_name n) : sc_module(n) {
  SC_THREAD(do_filter);
  sensitive << i_clk.pos();
  dont_initialize();
  reset_signal_is(i_rst, false);
}

void GaussianBlur::do_filter() {
  { wait(); }
  int i, j, k, l;
  //unsigned char[3] r, g, b;
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
  printf("%f\n",sum);

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
        red += i_r[u].read() * filter[v][u];
        green += i_g[u].read() * filter[v][u];
        blue += i_b[u].read() * filter[v][u];
      }
      wait();
    }
    o_red.write(red);
    o_green.write(green);
    o_blue.write(blue);
  }
}
