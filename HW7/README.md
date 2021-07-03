# Data Partitioning of the Gaussian Blur Processing

In this HW, we're going to add two Gaussian Blur modules to the dual-core riscv-vp platform "tiny32-mc". To achieve parallelism,the image is partitioned into equal parts and a multi-core program is written to issue the processing to the two modules.

## System Architecture

The system architecture is similar to the one in HW4, which uses the TLM transaction and a simpleBus to transfer the data in between as shown in the figure below. In addition, a direct memory access (DMA) module is involved to provide a faster data transfer.

<div align="center"> <img src="hw7.png" width="80%"/> </div>

To make it run parallelly, the 256x256 input matrix of bitmap is partitioned into two 128x256 matrixes. The way to partition the input data is as follows.
	
	int start_width = width / PROCESSORS * hart_id, end_width = width / PROCESSORS * hart_id + width / PROCESSORS;
    unsigned char  buffer[4] = {0};
    for(int i = start_width; i < end_width; i++){
		.
		.
		.
	}

The TLM simplebus shown above has 4 master modules and 7 slave modules. The TLM socket binding is as follows:

	core0_mem_if.isock.bind(bus.tsocks[0]);
	core1_mem_if.isock.bind(bus.tsocks[1]);
	dbg_if.isock.bind(bus.tsocks[2]);

	PeripheralWriteConnector dma_connector("SimpleDMA-Connector");  // to respect ISS bus locking
	dma_connector.isock.bind(bus.tsocks[3]);
	dma.isock.bind(dma_connector.tsock);
	dma_connector.bus_lock = bus_lock;

	bus.isocks[0].bind(mem.tsock);
	bus.isocks[1].bind(clint.tsock);
	bus.isocks[2].bind(sys.tsock);
	bus.isocks[3].bind(dma.tsock);
	bus.isocks[4].bind(gaussian_blur0.tsock);
	bus.isocks[5].bind(gaussian_blur1.tsock);
	bus.isocks[6].bind(plic.tsock);

## Modifications in codes

-	Most of the files remain the same as HW6. In `vp/platform`, another GaussianBlur module is added as mentioned above. In addition, the biggist changes I make lie in the `sw` directory.  
    
    A `bootstrap.s` file, which is written in assembly language, is added to handle the cooperation between two cores. Besides, in `main.cpp` semaphore locks and barrier are introduced to avoid race conditions and colliding between two cores. 

    The following codes shows the usage of semaphore locks with a pair of functions `sem_wait(&lock)` and `sem_post(&lock)`.

        unsigned char  buffer[4] = {0};
        for(int i = start_width; i < end_width; i++){
            for(int j = 0; j < length; j++){
            for(int v = -1; v <= 1; v ++){
                for(int u = -1; u <= 1; u++){
                if((v + i) >= 0  &&  (v + i ) < width && (u + j) >= 0 && (u + j) < length ){
                    buffer[0] = *(source_bitmap + bytes_per_pixel * ((j + u) * width + (i + v)) + 2);
                    buffer[1] = *(source_bitmap + bytes_per_pixel * ((j + u) * width + (i + v)) + 1);
                    buffer[2] = *(source_bitmap + bytes_per_pixel * ((j + u) * width + (i + v)) + 0);
                    buffer[3] = 0;
                }else{
                    buffer[0] = 0;
                    buffer[1] = 0;
                    buffer[2] = 0;
                    buffer[3] = 0;
                }
                sem_wait(&lock);
                if (hart_id == 0) {
                    write_data_to_Tiny32mc(GAUSSIAN0_START_ADDR, buffer, 4);
                } else {
                    write_data_to_Tiny32mc(GAUSSIAN1_START_ADDR, buffer, 4);
                }
                sem_post(&lock);
                }
            }
            
            sem_wait(&lock);
            if (hart_id == 0) {
                read_data_from_Tiny32mc(GAUSSIAN0_READ_ADDR, buffer, 4);
            } else {
                read_data_from_Tiny32mc(GAUSSIAN1_READ_ADDR, buffer, 4);
            }
            result[i][j][0] = buffer[0];
            result[i][j][1] = buffer[1];
            result[i][j][2] = buffer[2];
            result[i][j][3] = buffer[3];
            sem_post(&lock);
            }
        }

    The barrier is used to assure all of the cores have finished their jobs so that I can further print out some results as shown below.

        barrier(&barrier_sem, &barrier_lock, &barrier_counter, PROCESSORS);

        if (hart_id == 0) {  // Core 0 print first and then others
            sem_wait(&lock);
                for(int i = start_width; i < start_width+10; i++){
            for(int j = 0; j < 10; j++){
                printf ("[%d %d] %d %d %d %d\n",i,j,result[i][j][0],result[i][j][1],result[i][j][2],result[i][j][3]);
            }
            }
            sem_post(&lock);
            } else {
                for (int i = 1; i < PROCESSORS; ++i) {
            sem_wait(&lock);
            for(int i = start_width; i < start_width+10; i++){
                for(int j = 0; j < 10; j++){
                printf ("[%d %d] %d %d %d %d\n",i,j,result[i][j][0],result[i][j][1],result[i][j][2],result[i][j][3]);
                }
            }
            sem_post(&lock);
                }
            }
        }

-   Differ from the previous HW, we use a different core `tiny32-mc` here. Thus, the functions used to tranfer data between the application and the basic-acc platform with either DMA or simple memory are modified as below.

        void write_data_to_Tiny32mc(char* ADDR, unsigned char* buffer, int len){
            if(_is_using_dma){  
                // Using DMA 
                *DMA_SRC_ADDR = (uint32_t)(buffer);
                *DMA_DST_ADDR = (uint32_t)(ADDR);
                *DMA_LEN_ADDR = len;
                *DMA_OP_ADDR  = DMA_OP_MEMCPY;
            }else{
                // Directly Send
                memcpy(ADDR, buffer, sizeof(unsigned char)*len);
            }
        }
        void read_data_from_Tiny32mc(char* ADDR, unsigned char* buffer, int len){
            if(_is_using_dma){
                // Using DMA 
                *DMA_SRC_ADDR = (uint32_t)(ADDR);
                *DMA_DST_ADDR = (uint32_t)(buffer);
                *DMA_LEN_ADDR = len;
                *DMA_OP_ADDR  = DMA_OP_MEMCPY;
            }else{
                // Directly Read
                memcpy(buffer, ADDR, sizeof(unsigned char)*len);
            }
        }

## How to execute the codes

-	Build the `tiny32-mc` platform of RISC-V VP.

		$ cd HW7/vp
		$ mkdir build
		$ cd build
		$ cmake ..
		$ make install

-	Go to the working directory.

		$ cd HW7/sw

-	Compile and Run simulaitons.

		$ make
		$ make sim

## Results

-	The simulated result:

        SystemC 2.3.3-Accellera --- Jun 22 2021 19:36:11
        Copyright (c) 1996-2018 by all Contributors,
        ALL RIGHTS RESERVED

        Info: /OSCI/SystemC: Simulation stopped by user.
        =[ core : 0 ]===========================
        simulation time: 1764293080 ns
        zero (x0) =        0
        ra   (x1) =    10938
        sp   (x2) =    18a00
        gp   (x3) =    53104
        tp   (x4) =        0
        t0   (x5) =  2010000
        t1   (x6) =        1
        t2   (x7) =        1
        s0/fp(x8) =        0
        s1   (x9) =        0
        a0  (x10) =        0
        a1  (x11) =    538b4
        a2  (x12) =    538b0
        a3  (x13) =        2
        a4  (x14) =        1
        a5  (x15) =        0
        a6  (x16) =        0
        a7  (x17) =       5d
        s2  (x18) =        0
        s3  (x19) =        0
        s4  (x20) =        0
        s5  (x21) =        0
        s6  (x22) =        0
        s7  (x23) =        0
        s8  (x24) =        0
        s9  (x25) =        0
        s10 (x26) =        0
        s11 (x27) =        0
        t3  (x28) =        0
        t4  (x29) =        0
        t5  (x30) =     8800
        t6  (x31) =        5
        pc = 10964
        num-instr = 45436798
        =[ core : 1 ]===========================
        simulation time: 1764293080 ns
        zero (x0) =        0
        ra   (x1) =    10938
        sp   (x2) =    20a00
        gp   (x3) =    53104
        tp   (x4) =        0
        t0   (x5) =    20a00
        t1   (x6) =        1
        t2   (x7) =        1
        s0/fp(x8) =        0
        s1   (x9) =        0
        a0  (x10) =        0
        a1  (x11) =    538b4
        a2  (x12) =    538b0
        a3  (x13) =        2
        a4  (x14) =        1
        a5  (x15) =        0
        a6  (x16) =   525270
        a7  (x17) =    209c0
        s2  (x18) =        0
        s3  (x19) =        0
        s4  (x20) =        0
        s5  (x21) =        0
        s6  (x22) =        0
        s7  (x23) =        0
        s8  (x24) =        0
        s9  (x25) =        0
        s10 (x26) =        0
        s11 (x27) =        0
        t3  (x28) =        3
        t4  (x29) =        0
        t5  (x30) =     8800
        t6  (x31) =        5
        pc = 1094c
        num-instr = 45434145

-	Compare the result of single core version from HW6:

    | Configuration | Simulated time |
    | -----------   | -------------: |
    | Sinale core   |  3494287520 ns |
    | Dual-core     |  1764293080 ns |

    We can see that the simulated time of the dual-core version is almost exactly half of that of the single core, which is quite ideal.
