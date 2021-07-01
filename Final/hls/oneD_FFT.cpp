#include <cmath>
#ifndef NATIVE_SYSTEMC
#include "stratus_hls.h"
#endif
using namespace std;
using namespace sc_dt;
using namespace sc_core;
#include "fft_parameter.h"
#include "oneD_FFT.h"

oneD_FFT::oneD_FFT( sc_module_name n ): sc_module( n )
{
#ifndef NATIVE_SYSTEMC
	HLS_FLATTEN_ARRAY(W_real);
	HLS_FLATTEN_ARRAY(W_imag);
	HLS_FLATTEN_ARRAY(real);
	HLS_FLATTEN_ARRAY(imag);
#endif
	SC_THREAD( do_fft );
	sensitive << i_clk.pos();
	dont_initialize();
	reset_signal_is(i_rst, false);
        
#ifndef NATIVE_SYSTEMC
	i_real.clk_rst(i_clk, i_rst);
	i_imag.clk_rst(i_clk, i_rst);
	o_real.clk_rst(i_clk, i_rst);
	o_imag.clk_rst(i_clk, i_rst);
#endif
}

oneD_FFT::~oneD_FFT() {}

//Function for butterfly computation

 void butterfly
    ( const sc_int<16>& w_real  ,
      const sc_int<16>& w_imag  , 
      const sc_int<16>& real1_in,
      const sc_int<16>& imag1_in,
      const sc_int<16>& real2_in,
      const sc_int<16>& imag2_in,
      sc_int<16>& real1_out,
      sc_int<16>& imag1_out,
      sc_int<16>& real2_out,
      sc_int<16>& imag2_out
    )
 {

   // Variable declarations
     sc_int<17> tmp_real1;
     sc_int<17> tmp_imag1;
     sc_int<17> tmp_real2;
     sc_int<17> tmp_imag2;
     sc_int<34> tmp_real3;
     sc_int<34> tmp_imag3;
  
    // Begin Computation (fixed-point)
    // <s,6,10> = <s,5,10> + <s,5,10>
    tmp_real1 = real1_in + real2_in; 
    tmp_imag1 = imag1_in + imag2_in;
    tmp_real2 = real1_in - real2_in;
    tmp_imag2 = imag1_in - imag2_in;
    //   <s,13,20> = <s,6,10>*<s,5,10> - <s,6,10>*<s,5,10>
    tmp_real3 = tmp_real2*w_real - tmp_imag2*w_imag;
    //   <s,13,20> = <s,6,10>*<s,5,10> - <s,6,10>*<s,5,10>
    tmp_imag3 = tmp_real2*w_imag + tmp_imag2*w_real; 
    // assign the sign-bit(MSB)      
    real1_out[15] = tmp_real1[16];
    imag1_out[15] = tmp_imag1[16];
    // assign the rest of the bits
    real1_out.range(14,0) = tmp_real1.range(14,0);
    imag1_out.range(14,0) = tmp_imag1.range(14,0);
   // assign the sign-bit(MSB)      
    real2_out[15] = tmp_real3[33];
    imag2_out[15] = tmp_imag3[33];          
   // assign the rest of the bits
    real2_out.range(14,0) = tmp_real3.range(24,10);
    imag2_out.range(14,0) = tmp_imag3.range(24,10);

 } // end func_butterfly


void oneD_FFT::do_fft() {
	{
#ifndef NATIVE_SYSTEMC
		HLS_DEFINE_PROTOCOL("main_reset");
		i_real.reset();
		i_imag.reset();
		o_real.reset();
		o_imag.reset();
#endif
		wait();
	}
	  sc_int<16> tmp_real;
	  sc_int<16> tmp_imag;
	  
	  sc_int<8> len;
	  sc_int<16> w_real;
	  sc_int<16> w_imag;
	  sc_int<16> w_rec_real;
	  sc_int<16> w_rec_imag;
	  sc_int<32> w_temp1;
	  sc_int<32> w_temp2;
	  sc_int<32> w_temp3;
	  sc_int<32> w_temp4;
	  sc_int<33> w_temp5;
	  sc_int<33> w_temp6;
	  sc_int<16> real1_in;
	  sc_int<16> imag1_in;
	  sc_int<16> real2_in;
	  sc_int<16> imag2_in;
	  sc_int<16> real1_out;
	  sc_int<16> imag1_out;
	  sc_int<16> real2_out;
	  sc_int<16> imag2_out;
	  sc_int<4> stage;
	  sc_uint<4> bits_i;
	  sc_uint<4> bits_index;
	  sc_int<16> result_real, result_imag;
	  int index, index2, windex, incr;

	while (true) {
	//data_req.write(false);
    //data_ready.write(false);
    index = 0; 
     
    wait();
    //Read in the Sample values
      cout << endl << "Reading in the samples..." << endl;
      while( index < 16 )
      {
       //data_req.write(true);
       //do { wait(); } while ( !(data_valid == true) );
#ifndef NATIVE_SYSTEMC
	{
		HLS_DEFINE_PROTOCOL("input");
		tmp_real = i_real.get();
		tmp_imag = i_imag.get();
	}
#else
		tmp_real = i_real.read();
       	tmp_imag = i_imag.read();
#endif
       real[index] = tmp_real;
       imag[index] = tmp_imag;
       cout << "Input received" << endl;
       index++;
       //data_req.write(false);
       wait();
      }
      index = 0;
		for (unsigned int i = 0; i<W; ++i) {
			HLS_CONSTRAIN_LATENCY(0, 1, "lat00");
			W_real[W] = 0;
			W_imag[W] = 0;
		}

		cout << "Computing TFs" << endl;
	     // Calculate the W-values recursively
	     // <'s'/'u',m,n>: is used in comments to denote a fixed point representation
	     // 's'- signed, 'u'- unsigned, m - no. of integer bits, n - no. of fractional bits
	     //  N = 16
	     //  theta = 8.0*atan(1.0)/N; theta = 22.5 degree

       //  w_real =  cos(theta) = 0.92 (000000.1110101110) <s,5,10>
           w_real =  942;

       //  w_imag = -sin(theta) = -0.38(111111.1001111010) <s,5,10>
           w_imag = -389;

       //  w_rec_real = 1(0000001.0000000000)
	   	   w_rec_real = 1024;

       //  w_rec_real = 0(000000.0000000000)	 
           w_rec_imag = 0;

	    unsigned short w_index;
	    w_index = 0;  
	    for( w_index = 0; w_index < W; ++w_index) 
	    {
	    // <s,9,22> = <s,5,10> * <s,5,10>
	     w_temp1 = w_rec_real*w_real;
	     w_temp2 = w_rec_imag*w_imag;

	    // <s,9,22> = <s,5,10> * <s,5,10>
	     w_temp3 = w_rec_real*w_imag;
	     w_temp4 = w_rec_imag*w_real;  

	    // <s,10,22> = <s,9,22> - <s,9,22>
	     w_temp5 = w_temp1 - w_temp2;

	    // <s,10,22> = <s,9,22> + <s,9,22>
	     w_temp6 = w_temp3 + w_temp4;
	     
	    // assign the sign-bit(MSB)
	     W_real[w_index][15] = w_temp5[32];
	     W_imag[w_index][15] = w_temp6[32];

	    // assign the rest of the bits
	     W_real[w_index].range(14,0) = w_temp5.range(24,10);
	     W_imag[w_index].range(14,0) = w_temp6.range(24,10);

	    // update w_rec.. values for the next iteration
	     w_rec_real = W_real[w_index];
	     w_rec_imag = W_imag[w_index];

	     }

	  /////////////////////////////////////////////////////////////////
      ///  Computation - 1D Complex DIF FFT Computation Algorithm  ////
      /////////////////////////////////////////////////////////////////

	      stage = 0;
	      len = N;
	      incr = 1;
	      //cout << "FFT starts" << endl;
	      while (stage < M) 
	      { 
	        len = len >> 1;
	        //First Iteration :  Simple calculation, with no multiplies
	          int k = 0;
	          while(k < N)  
	          {
	             index =  k; index2 = k + len; 
	             tmp_real = real[index] + real[index2];
	             tmp_imag = imag[index] + imag[index2];
	             real[index2] = (real[index] - real[index2]);
	             imag[index2] = (imag[index] - imag[index2]);
	             real[index] = tmp_real;
	             imag[index] = tmp_imag;
	      
	             k = k + (len << 1);  
	             //cout << "k = "<< k << endl; 
	          }
	          //cout << "real = "<< tmp_real << endl;

	        //Remaining Iterations: Use Stored W
	         int l = 1;
	         windex = incr - 1;
	        // This loop executes N/2 times at the first stage, N/2 times at the second.. once at last stage
	         while (l < len)
	         {
	            int m = l; 
	            while (m < N) 
	            {
	              index = m;
	              index2 = m + len;

	              // Read in the data and twiddle factors
	              w_real  = W_real[windex];
	              w_imag  = W_imag[windex];

	              real1_in = real[index];
	              imag1_in = imag[index];
	              real2_in = real[index2];
	              imag2_in = imag[index2];

	              // Call butterfly computation function       
	              butterfly(w_real, w_imag, real1_in, imag1_in, real2_in, imag2_in, real1_out, imag1_out, real2_out, imag2_out);

	              // Store back the results
	              real[index]  = real1_out;
	              imag[index]  = imag1_out; 
	              real[index2] = real2_out;
	              imag[index2] = imag2_out; 

	              m = m + (len << 1);
	            }
	            windex = windex + incr;
	            l++;
	          }
	          stage++;
	          incr = incr << 1;
	          //cout << "stage = "<< stage << endl;
	      } 
	           
	     //////////////////////////////////////////////////////////////////////////   
	     //Writing out the normalized transform values in bit reversed order    ///
	     //////////////////////////////////////////////////////////////////////////
	      sc_int<16> real1;
      	  sc_int<16> imag1;
	      bits_i = 0;
	      bits_index = 0;

	      //cout << "Writing the transform values..." << endl;
	      for (int n = 0; n < N; ++n)
	      {
	       bits_i = n;
	       bits_index[3]= bits_i[0];
	       bits_index[2]= bits_i[1];
	       bits_index[1]= bits_i[2];
	       bits_index[0]= bits_i[3];
	       index = bits_index;
	       real1 = real[index];
	       imag1 = imag[index];
#ifndef NATIVE_SYSTEMC
		{
		   HLS_DEFINE_PROTOCOL("output");
		   o_real.put(real1); 
	       o_imag.put(imag1);
		   cout << "Writing output" << endl;
		}
#else
		   o_real.write(real1); 
	       o_imag.write(imag1);
#endif
	       //data_ready.write(true);
	       //do { wait(); } while ( !(data_ack == true) );
	       //data_ready.write(false);
	       wait();
	      }
	      index = 0; 
	      cout << "FFT done..." << endl;
  }
}
