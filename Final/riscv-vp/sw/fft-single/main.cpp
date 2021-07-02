#include <iostream>
#include <ostream>
#include <string.h>
#include "stdio.h"
using namespace std;

// 2DFFT ACC
static char* const TWOD_FFT_START_ADDR = reinterpret_cast<char* const>(0x80000000);
static char* const TWOD_FFT_START1_ADDR  = reinterpret_cast<char* const>(0x80000004);
static char* const TWOD_FFT_READ_ADDR = reinterpret_cast<char* const>(0x80000008);
static char* const TWOD_FFT_READ1_ADDR  = reinterpret_cast<char* const>(0x8000000C);
// DMA 
static volatile uint32_t * const DMA_SRC_ADDR  = (uint32_t * const)0x70000000;
static volatile uint32_t * const DMA_DST_ADDR  = (uint32_t * const)0x70000004;
static volatile uint32_t * const DMA_LEN_ADDR  = (uint32_t * const)0x70000008;
static volatile uint32_t * const DMA_OP_ADDR   = (uint32_t * const)0x7000000C;
static volatile uint32_t * const DMA_STAT_ADDR = (uint32_t * const)0x70000010;
static const uint32_t DMA_OP_MEMCPY = 1;

bool _is_using_dma = true;

unsigned int ReadfromByteArray(unsigned char* array, unsigned int offset) {
	unsigned int output = (array[offset] << 0) | (array[offset + 1] << 8) | (array[offset + 2] << 16) | (array[offset + 3] << 24);
	return output;
}

union word{
    int sint;
    char character[4];
};

void write_data_to_ACC(char* ADDR, int buffer, int len){
  word data;
	unsigned char buff[4];
	
	data.sint = buffer;
	buff[0] = data.character[0];
  buff[1] = data.character[1];
  buff[2] = data.character[2];
  buff[3] = data.character[3];
  
  if(_is_using_dma){  
    // Using DMA 
    *DMA_SRC_ADDR = (uint32_t)(buff);
    *DMA_DST_ADDR = (uint32_t)(ADDR);
    *DMA_LEN_ADDR = len;
    *DMA_OP_ADDR  = DMA_OP_MEMCPY;
  }else{
    // Directly Send
    memcpy(ADDR, buff, sizeof(unsigned char)*len);
  }
}
int read_data_from_ACC(char* ADDR, int len){
  word data;
	unsigned char buff[4];

  if(_is_using_dma){
    // Using DMA 
    *DMA_SRC_ADDR = (uint32_t)(ADDR);
    *DMA_DST_ADDR = (uint32_t)(buff);
    *DMA_LEN_ADDR = len;
    *DMA_OP_ADDR  = DMA_OP_MEMCPY;
  }else{
    // Directly Read
    memcpy(buff, ADDR, sizeof(unsigned char)*len);
  }
  data.character[0] = buff[0]; 
  data.character[1] = buff[1];
  data.character[2] = buff[2];
  data.character[3] = buff[3];
  //printf("%d",buff[0]);
  return data.sint;
}

int main() {
  #include "in_real.h" //included here to avoid compiler issue of not initializing global arrays
  #include "in_imag.h"
  word temp_r,temp_i;
  int buffer_r[4] = {0};
  int buffer_i[4] = {0};
  int out_r[16] = {0};
  int out_i[16] = {0};
  //int buffer_r= 0; int buffer_i =0; int out_r = 0; int out_i = 0;
  int length = 16;
  for(int i = 0; i < length; i++){
    temp_r.sint = input_r[i];
    temp_i.sint = input_i[i];
    buffer_r[0] = temp_r.character[0];
    buffer_r[1] = temp_r.character[1];
    buffer_r[2] = temp_r.character[2];
    buffer_r[3] = temp_r.character[3];

    buffer_i[0] = temp_i.character[0];
    buffer_i[1] = temp_i.character[1];
    buffer_i[2] = temp_i.character[2];
    buffer_i[3] = temp_i.character[3];
    cout << "Sending input" << endl;
    cout << "input_real = "<< input_r[i] << endl;
    cout << "input_imag = "<< input_i[i] << endl;
    write_data_to_ACC(TWOD_FFT_START_ADDR, input_r[i], 4);
    write_data_to_ACC(TWOD_FFT_START1_ADDR, input_i[i], 4);
  }
  for(int j = 0; j < length; j++){
    out_r[j] = read_data_from_ACC(TWOD_FFT_READ_ADDR, 4);
    out_i[j] = read_data_from_ACC(TWOD_FFT_READ1_ADDR, 4);
    cout << "Out real =" << out_r[j] << endl;
    cout << "Out imag =" << out_i[j] << endl;
  }
}
