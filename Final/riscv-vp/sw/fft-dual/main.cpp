//#include <iostream>
//#include <ostream>
#include <string.h>
#include "stdio.h"
using namespace std;

int sem_init (uint32_t *__sem, uint32_t count) __THROW
{
  *__sem=count;
  return 0;
}

int sem_wait (uint32_t *__sem) __THROW
{
  uint32_t value, success; //RV32A
  __asm__ __volatile__("\
L%=:\n\t\
     lr.w %[value],(%[__sem])            # load reserved\n\t\
     beqz %[value],L%=                   # if zero, try again\n\t\
     addi %[value],%[value],-1           # value --\n\t\
     sc.w %[success],%[value],(%[__sem]) # store conditionally\n\t\
     bnez %[success], L%=                # if the store failed, try again\n\t\
"
    : [value] "=r"(value), [success]"=r"(success)
    : [__sem] "r"(__sem)
    : "memory");
  return 0;
}

int sem_post (uint32_t *__sem) __THROW
{
  uint32_t value, success; //RV32A
  __asm__ __volatile__("\
L%=:\n\t\
     lr.w %[value],(%[__sem])            # load reserved\n\t\
     addi %[value],%[value], 1           # value ++\n\t\
     sc.w %[success],%[value],(%[__sem]) # store conditionally\n\t\
     bnez %[success], L%=                # if the store failed, try again\n\t\
"
    : [value] "=r"(value), [success]"=r"(success)
    : [__sem] "r"(__sem)
    : "memory");
  return 0;
}

int barrier(uint32_t *__sem, uint32_t *__lock, uint32_t *counter, uint32_t thread_count) {
	sem_wait(__lock);
	if (*counter == thread_count - 1) { //all finished
		*counter = 0;
		sem_post(__lock);
		for (int j = 0; j < thread_count - 1; ++j) sem_post(__sem);
	} else {
		(*counter)++;
		sem_post(__lock);
		sem_wait(__sem);
	}
	return 0;
}

// 2DFFT Tiny32-mc
static char* const TWOD_FFT0_START_ADDR = reinterpret_cast<char* const>(0x30000000);
static char* const TWOD_FFT0_START1_ADDR  = reinterpret_cast<char* const>(0x30000004);
static char* const TWOD_FFT0_READ_ADDR = reinterpret_cast<char* const>(0x30000008);
static char* const TWOD_FFT0_READ1_ADDR  = reinterpret_cast<char* const>(0x3000000C);
static char* const TWOD_FFT1_START_ADDR = reinterpret_cast<char* const>(0x40000000);
static char* const TWOD_FFT1_START1_ADDR  = reinterpret_cast<char* const>(0x40000004);
static char* const TWOD_FFT1_READ_ADDR = reinterpret_cast<char* const>(0x40000008);
static char* const TWOD_FFT1_READ1_ADDR  = reinterpret_cast<char* const>(0x4000000C);

// DMA 
static volatile uint32_t * const DMA_SRC_ADDR  = (uint32_t * const)0x50000000;
static volatile uint32_t * const DMA_DST_ADDR  = (uint32_t * const)0x50000004;
static volatile uint32_t * const DMA_LEN_ADDR  = (uint32_t * const)0x50000008;
static volatile uint32_t * const DMA_OP_ADDR   = (uint32_t * const)0x5000000C;
static volatile uint32_t * const DMA_STAT_ADDR = (uint32_t * const)0x50000010;
static const uint32_t DMA_OP_MEMCPY = 1;

// Switch between DMA & memory
bool _is_using_dma = true;

unsigned int ReadfromByteArray(unsigned char* array, unsigned int offset) {
	unsigned int output = (array[offset] << 0) | (array[offset + 1] << 8) | (array[offset + 2] << 16) | (array[offset + 3] << 24);
	return output;
}

union word{
    int sint;
    char character[4];
};

void write_data_to_Tiny32mc(char* ADDR, int buffer, int len){
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
int read_data_from_Tiny32mc(char* ADDR, int len){
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


//Total number of cores
//static const int PROCESSORS = 2;
#define PROCESSORS 2
//the barrier synchronization objects
uint32_t barrier_counter=0; 
uint32_t barrier_lock; 
uint32_t barrier_sem; 
//the mutex object to control global summation
uint32_t lock;  
//print synchronication semaphore (print in core order)
uint32_t print_sem[PROCESSORS];
//unsigned char result[260][260][4];

int main(unsigned hart_id) {
  #include "in_real.h" //included here to avoid compiler issue of not initializing global arrays
  #include "in_imag.h"
  /////////////////////////////
	// thread and barrier init //
	/////////////////////////////
	//printf("Core%d\n", hart_id);
	if (hart_id == 0) {
		// create a barrier object with a count of PROCESSORS
		sem_init(&barrier_lock, 1);
		sem_init(&barrier_sem, 0); //lock all cores initially
		for(int i=0; i< PROCESSORS; ++i){
			sem_init(&print_sem[i], 0); //lock printing initially
		}
		// Create mutex lock
		sem_init(&lock, 1);
	}
  int length = 16;
  int start_length = length / PROCESSORS * hart_id, end_length = length / PROCESSORS * hart_id + length / PROCESSORS;
  word temp_r,temp_i;
  int buffer_r[4] = {0};
  int buffer_i[4] = {0};
  int out_r[16] = {0};
  int out_i[16] = {0};
  for(int i = start_length; i < end_length; i++){
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
    
    //cout << "Sending input" << endl;
    
    //cout << "input_real = "<< input_r[i] << endl;
    //cout << "input_imag = "<< input_i[i] << endl;
    sem_wait(&lock);
    if (hart_id == 0) {
      write_data_to_Tiny32mc(TWOD_FFT0_START_ADDR, input_r[i], 4);
      write_data_to_Tiny32mc(TWOD_FFT0_START1_ADDR, input_i[i], 4);
      //printf("i = %d\n",i);
      printf("input real = %d\t\n", input_r[i]);
      printf("input imag = %d\t\n", input_i[i]);
    } else {
      write_data_to_Tiny32mc(TWOD_FFT1_START_ADDR, input_r[i], 4);
      write_data_to_Tiny32mc(TWOD_FFT1_START1_ADDR, input_i[i], 4);
      //printf("input real = %d\t\n", input_r[i]);
      //printf("input imag = %d\t\n", input_i[i]);
    }
    sem_post(&lock);
  }
  for(int j = start_length; j < end_length; j++){
    sem_wait(&lock);
    if (hart_id == 0) {
      out_r[j] = read_data_from_Tiny32mc(TWOD_FFT0_READ_ADDR, 4);
      out_i[j] = read_data_from_Tiny32mc(TWOD_FFT0_READ1_ADDR, 4);
      //printf("out real = %d\t\n", out_r[j]);
      //printf("out imag = %d\t\n", out_i[j]);
    } else {
      out_r[j] = read_data_from_Tiny32mc(TWOD_FFT1_READ_ADDR, 4);
      out_i[j] = read_data_from_Tiny32mc(TWOD_FFT1_READ1_ADDR, 4);
      //cout << "Out real =" << out_r[j] << endl;
      //cout << "Out imag =" << out_i[j] << endl;
    }
    sem_post(&lock);
  }

  ////////////////////////////
  // barrier to synchronize //
  ////////////////////////////
  //Wait for both threads to finish
  barrier(&barrier_sem, &barrier_lock, &barrier_counter, PROCESSORS);

  if (hart_id == 0) {  // Core 0 print first and then others
    sem_wait(&lock);
		for(int k = start_length; k < end_length; k++){
      //cout << "Out real =" << out_r[k] << endl;
      //cout << "Out imag =" << out_i[k] << endl;
    }
    sem_post(&lock);
	} else {
    sem_wait(&lock);
    for(int l = start_length; l < end_length; l++){
    //cout << "Out real =" << out_r[l] << endl;
    //cout << "Out imag =" << out_i[l] << endl;
    }
    sem_post(&lock);
	}

	return 0;
}
