# Cross-compile Gaussian Blur to RISC-V VP platform

In this HW, we're going to port the Gaussian blur module to the "basic-acc" platform, and simulate the behavior that a program runs on the RISC-V single core platform.


## System Architecture

The system architecture is similar to the one in HW4, which uses the TLM transaction and a simpleBus to transfer the data in between.

The TLM simplebus shown above has 3 master modules and 13 slave modules. The TLM socket binding is as follows:

	iss_mem_if.isock.bind(bus.tsocks[0]);
	dbg_if.isock.bind(bus.tsocks[2]);

	PeripheralWriteConnector dma_connector("SimpleDMA-Connector");  // to respect ISS bus locking
	dma_connector.isock.bind(bus.tsocks[1]);
	dma.isock.bind(dma_connector.tsock);
	dma_connector.bus_lock = bus_lock;

	bus.isocks[0].bind(mem.tsock);
	bus.isocks[1].bind(clint.tsock);
	bus.isocks[2].bind(plic.tsock);
	bus.isocks[3].bind(term.tsock);
	bus.isocks[4].bind(sensor.tsock);
	bus.isocks[5].bind(dma.tsock);
	bus.isocks[6].bind(sensor2.tsock);
	bus.isocks[7].bind(mram.tsock);
	bus.isocks[8].bind(flashController.tsock);
	bus.isocks[9].bind(ethernet.tsock);
	bus.isocks[10].bind(display.tsock);
	bus.isocks[11].bind(sys.tsock);
	bus.isocks[12].bind(gaussian_blur.tsock);

## Implementations

-	Similar to HW4, the data is transfered with TLM blocking transport. The relevant code can be found in `vp/platform/main.cpp` as follows.

        void blocking_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay){
        wait(delay);
        tlm::tlm_command cmd = payload.get_command();
        sc_dt::uint64 addr = payload.get_address();
        unsigned char *data_ptr = payload.get_data_ptr();

        addr -= base_offset;
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
                std::cerr << "READ Error! GaussianBlur::blocking_transport: address 0x"
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
                std::cerr << "WRITE Error! GaussianBlur::blocking_transport: address 0x"
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

-   The data is read/write from/to the `GaussianBlur` in the way below, which can be found in `sw/main.cpp`.

        unsigned char  buffer[4] = {0};
        for(int i = 0; i < width; i++){
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
                    write_data_to_ACC(SOBELFILTER_START_ADDR, buffer, 4);
                    }
                }
                read_data_from_ACC(SOBELFILTER_READ_ADDR, buffer, 4);
            }
        }

    The functions used to tranfer data between the application and the basic-acc platform with either DMA or simple memory are defined as below.

        void write_data_to_ACC(char* ADDR, unsigned char* buffer, int len){
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

        void read_data_from_ACC(char* ADDR, unsigned char* buffer, int len){
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

-	Build the `basic-acc` platform of RISC-V VP.

		$ cd HW6/vp
		$ mkdir build
		$ cd build
		$ cmake ..
		$ make install

-	Go to the working directory.

		$ cd HW6/sw

-	Compile and Run simulaitons.

		$ make
		$ make sim

## Results

-	The simulated result:

            SystemC 2.3.3-Accellera --- Jun 17 2021 16:50:52
            Copyright (c) 1996-2018 by all Contributors,
            ALL RIGHTS RESERVED
        ======================================
            Reading from array
        ======================================
        input_rgb_raw_data_offset	= 54
        width				= 256
        length				= 256
        bytes_per_pixel		= 3
        ======================================

        Info: /OSCI/SystemC: Simulation stopped by user.
        =[ core : 0 ]===========================
        simulation time: 3494287520 ns
        zero (x0) =        0
        ra   (x1) =    10696
        sp   (x2) =  1ffffec
        gp   (x3) =    508f0
        tp   (x4) =        0
        t0   (x5) =       20
        t1   (x6) =    30000
        t2   (x7) =        1
        s0/fp(x8) =        0
        s1   (x9) =        0
        a0  (x10) =        0
        a1  (x11) =        0
        a2  (x12) =      4c1
        a3  (x13) =        0
        a4  (x14) =        0
        a5  (x15) =        0
        a6  (x16) =        1
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
        t3  (x28) =        3
        t4  (x29) =        2
        t5  (x30) =     8800
        t6  (x31) =        5
        pc = 1b6a8
        num-instr = 94200746
        
