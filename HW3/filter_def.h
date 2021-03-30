#ifndef FILTER_DEF_H_
#define FILTER_DEF_H_

const int filterWidth = 3;
const int filterHeight = 3;
const double factor = 1.0;
const double bias = 0.0;

const int GAUSSIAN_FILTER_R_ADDR = 0x00000000;
const int GAUSSIAN_FILTER_RESULT_ADDR = 0x00000004;
const int GAUSSIAN_FILTER_RESULT_ADDR2 = 0x00000008;
const int GAUSSIAN_FILTER_RESULT_ADDR3 = 0x0000000c;

union word {
  int sint;
  unsigned int uint;
  unsigned char uc[4];
};

#endif
