#ifndef FILTER_DEF_H_
#define FILTER_DEF_H_

const int filterWidth = 3;
const int filterHeight = 3;
const double factor = 1.0;
const double bias = 0.0;

const int GAUSSIAN_FILTER_R_ADDR = 0x00000000;
const int GAUSSIAN_FILTER_RESULT_ADDR = 0x00000004;

union word {
  int sint[4];
  unsigned int uint[4];
  unsigned char uc[4];
};

#endif
