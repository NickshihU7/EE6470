# Final Project - 1D Fast Fourier Transform (1D FFT)

In the final project, I'm going to implement a 16-pt FFT module with a multi-core riscv-vp platform.  
In this project, I implemented the FFT core module on both single core platform and multi-core platform to compare the performance between w/ and w/o parallelism. In addition, to get the exact simulated time of the FFT core, a HLS FFT module is also implemented.

## The algorithm of DIF FFT

FFT is an algorithm that computes the discrete Fourier transform (DFT) of a sequence, or its inverse (IDFT). Fourier analysis converts a signal from its original domain (usually time or space) to a representation in the frequency domain and vice versa.

The formula for the DFT is: 

![DFT Formula](https://latex.codecogs.com/svg.image?X%5E%7Bd%7D%5Cleft%20(%20k%20%5Cright%20)=%5Csum_%7Bn=0%7D%5E%7BN-1%7Dx%5Cleft%20(%20n%20%5Cright%20)W_%7Bnk%7D%5E%7BN%7D,%200%5Cleq%20k%5Cleq%20N-1%5C)

where the twiddle factor (TF)

![TF](https://latex.codecogs.com/svg.image?W_{nk}^{N}=exp\left&space;(&space;-j\frac{2\pi}{N}nk&space;\right&space;))

The DFT thus has the time complexity of N^2, where the N is the number of inputs, which Hence is inefficient for hardware.

Thus, the FFT was proposed for the purpose of making it efficient and more hardware suitable. The decimation-in-frequency (DIF) FFT algorithm is: 

![DIF FFT](https://latex.codecogs.com/svg.image?X\left&space;(&space;k&space;\right&space;)=X\left&space;(&space;2m&space;\right&space;)&plus;X\left&space;(&space;2m&plus;1&space;\right&space;))

![DIF FFT](https://latex.codecogs.com/svg.image?=\sum_{n=0}^{N/2-1}\left&space;[&space;x\left&space;(&space;n&space;\right&space;)&plus;x\left&space;(&space;n&plus;\frac{N}{2}&space;\right&space;)&space;\right&space;]W_{N/2}^{mn}&plus;\sum_{n=0}^{N/2-1}\left&space;(&space;\left&space;[&space;x\left&space;(&space;n&space;\right&space;)-x\left&space;(&space;n&plus;\frac{N}{2}&space;\right&space;)&space;\right&space;]W_{N/2}^{mn}&space;\right&space;)W_{N}^{n})

which devides the output terms into even and odd. By doing so on and on for each stage, finally it has the time somplexity of Nlog2(N), which significantly decreasse the latency.

The relative architecture design for 16-pt DIF FFT which has 4 stages is shown in the figure below.

<div align="center"> <img src="DIF_FFT.png" width="80%"/> </div>

Each cross in the figure above is the basic computation unit of this architecture, which is called "Butterfly." Each butterfly has 2 complex inputs and 2 complex outputs, which involves in a multiplication with TFs and 2 additions. The block diagram of it is shown below.

<div align="center"> <img src="butterfly.png" width="30%"/> </div>

The order of inputs in this DIF FFT algorithm is in order. Nevertheless, the ouputs are in bit-reversed order. Thus, we'll need a re-order module to get the outputs in order.

## System Architecture for Dual-core System

The system architecture is similar to the one in HW4, which uses the TLM transaction and a simpleBus to transfer the data in between as shown in the figure below. In addition, a direct memory access (DMA) module is involved to provide a faster data transfer.

![The system architecture](final.png)

To make it run parallelly, the 16-pt inputs are splited into two 8-pt inputs for both real and imaginary parts. In addition, the 16-pts DIF FFT is replaced with two 8-pt DIF FFT.

1. 	A top-level module `System` is instantiated to contain `Testbench` and `GaussianBlur`.
2. 	Compared to HW2, the three R, G, and B channels are combined as one channel "rgb".
3. 	Instead of `sc_fifo`, the data channels are defined as the synthesizable streaming interface `cynw_p2p<>` of Stratus HLS.
4. 	The directory stratus contains the Stratus HLS project file (project.tcl) and Makefile to run Stratus HLS.

## Implementations

1.	In the system.h:

		#ifndef NATIVE_SYSTEMC
			GaussianBlur_wrapper gaussian_blur;
		#else
			GaussianBlur gaussian_blur;
		#endif
			sc_clock clk;
			sc_signal<bool> rst;
		#ifndef NATIVE_SYSTEMC
			cynw_p2p< sc_dt::sc_uint<24> > rgb;
			cynw_p2p< sc_dt::sc_uint<32> > red;
			cynw_p2p< sc_dt::sc_uint<32> > green;
			cynw_p2p< sc_dt::sc_uint<32> > blue;
		#else
			sc_fifo< sc_dt::sc_uint<24> > rgb;
			sc_fifo< sc_dt::sc_uint<32> > red;
			sc_fifo< sc_dt::sc_uint<32> > green;
			sc_fifo< sc_dt::sc_uint<32> > blue;
		#endif

2.	In the Testbench.h:

		#ifndef NATIVE_SYSTEMC
			cynw_p2p< sc_dt::sc_uint<24> >::base_out o_rgb;
			cynw_p2p< sc_dt::sc_uint<32> >::base_in i_red;
			cynw_p2p< sc_dt::sc_uint<32> >::base_in i_green;
			cynw_p2p< sc_dt::sc_uint<32> >::base_in i_blue;
		#else
			sc_fifo_out< sc_dt::sc_uint<24> > o_rgb;
			sc_fifo_in< sc_dt::sc_uint<32> > i_red;
			sc_fifo_in< sc_dt::sc_uint<32> > i_green;
			sc_fifo_in< sc_dt::sc_uint<32> > i_blue;
		#endif

3.	In the GaussianBlur.cpp:

		#ifndef NATIVE_SYSTEMC
			cynw_p2p< sc_dt::sc_uint<24> >::in i_rgb;
			cynw_p2p< sc_dt::sc_uint<32> >::out o_red;
			cynw_p2p< sc_dt::sc_uint<32> >::out o_green;
			cynw_p2p< sc_dt::sc_uint<32> >::out o_blue;
		#else
			sc_fifo_in< sc_dt::sc_uint<24> > i_rgb;
			sc_fifo_out< sc_dt::sc_uint<32> > o_red;
			sc_fifo_out< sc_dt::sc_uint<32> > o_green;
			sc_fifo_out< sc_dt::sc_uint<32> > o_blue;
		#endif

4.	In the Testbench.cpp and GaussianBlur.cpp, the read() and write() functions are replaced with get() and put() functions. For example:

		#ifndef NATIVE_SYSTEMC
			o_rgb.put(rgb);
		#else
			o_rgb.write(rgb);
		#endif
		#ifndef NATIVE_SYSTEMC
			red = i_red.get();
		    green = i_green.get();
		    blue = i_blue.get();
		#else
			red = i_red.read();
		    green = i_green.read();
		    blue = i_blue.read();
		#endif

## How to execute the codes

### HLS simulation

-	First of all, go to the stratus directory.

		$ cd $Final/hls/stratus

-	Run behavioral simulation.

		$ make sim_B

-	Run synthesis and Verilog simulation with HLS configuration BASIC.

		$ make sim_V_BASIC

-	Run synthesis and Verilog simulation with HLS configuration DPA.

		$ make sim_V_DPA

### RISC-V VP

-	Build the platforms of RISC-V VP.

		$ cd Final/riscv-vp/vp
		$ mkdir build
		$ cd build
		$ cmake ..
		$ make install

-	Go to one of the following working directory.

		$ cd Final/riscv-vp/vp/sw/singlecore
		$ cd Final/riscv-vp/vp/sw/multicore

-	Compile and Run simulaitons.

		$ make
		$ make sim

## Results

-	The 16-pt inputs lie in the files `in_real.h` and `in_imag.h` are shown below.
		
		int input_r [16] = {
			1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

		int input_i [16] = {
			2021,2021,2021,2021,2021,2021,2021,2021,
			2021,2021,2021,2021,2021,2021,2021,2021};

-	The outputs I got are as follows which are as expected.

		out_real = 1, out_imag = 32336
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0
		out_real = 1, out_imag = 0

-	The simulated results are as follows:

	-	The HLS behavioral simulation.

			    SystemC 2.3.1-Accellera --- Feb 14 2019 12:08:38
	        Copyright (c) 1996-2014 by all Contributors,
	        ALL RIGHTS RESERVED
			NOTE: Cadence Design Systems Hub Simulation Platform : version 19.12-s100

			Info: /OSCI/SystemC: Simulation stopped by user.
			Total run time = 2621430 ns
			Simulated time == 2621490 ns

	-	The single core version.

	-	The dual-core version.

	-	To sum up:

		| Configuration | Simulated time |
		| -----------   | -------------: |
		| HLS sim_B    	|     2621430 ns |
		| Sinale core   |    43909110 ns |
		| Dual-core     |    38010870 ns |
