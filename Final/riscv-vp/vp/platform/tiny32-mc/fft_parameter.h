#ifndef FILTER_DEF_H_
#define FILTER_DEF_H_

#define CLOCK_PERIOD 10


#define MAX_IMAGE_BUFFER_LENTH 1024


// Sobel Filter inner transport addresses
// Used between blocking_transport() & do_filter()
const int TWOD_FFT_R_ADDR = 0x00000000;
const int TWOD_FFT_R1_ADDR = 0x00000004;
const int TWOD_FFT_RESULT_ADDR = 0x00000008;
const int TWOD_FFT_RESULT1_ADDR = 0x0000000C;


union word {
  int sint;
  unsigned int uint;
  char character[4];
};

const int M = 3; //Stages
const int N = 8; //# of points
const int W = N/2 - 1; //# of TF

#endif
