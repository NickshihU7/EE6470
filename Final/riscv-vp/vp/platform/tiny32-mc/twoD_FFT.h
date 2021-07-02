#include <systemc>
#include <cmath>
#include <iomanip>
using namespace sc_core;
using namespace std;
using namespace sc_dt;

#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "fft_parameter.h"

struct twoD_FFT : public sc_module {
  tlm_utils::simple_target_socket<twoD_FFT> tsock;

  sc_fifo<int> i_real;
  sc_fifo<int> i_imag;
  sc_fifo<int> o_real;
  sc_fifo<int> o_imag;

  SC_HAS_PROCESS(twoD_FFT);

  twoD_FFT(sc_module_name n): 
    sc_module(n), 
    tsock("t_skt"), 
    base_offset(0) 
  {
    tsock.register_b_transport(this, &twoD_FFT::blocking_transport);
    SC_THREAD(do_fft);
  }

  ~twoD_FFT() {
	}

  unsigned int base_offset;

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
      // <s,5,11> = <s,4,11> + <s,4,11>
      tmp_real1 = real1_in + real2_in; 
      tmp_imag1 = imag1_in + imag2_in;
      tmp_real2 = real1_in - real2_in;
      tmp_imag2 = imag1_in - imag2_in;
      //   <s,11,22> = <s,5,11>*<s,4,11> - <s,5,11>*<s,4,11>
      tmp_real3 = tmp_real2*w_real - tmp_imag2*w_imag;
      //   <s,11,22> = <s,5,11>*<s,4,11> - <s,5,11>*<s,4,11>
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
      real2_out.range(14,0) = tmp_real3.range(25,11);
      imag2_out.range(14,0) = tmp_imag3.range(25,11);

  } // end func_butterfly

  void do_fft(){
    { wait(CLOCK_PERIOD, SC_NS); }
    sc_int<16> real[N];
    sc_int<16> imag[N];
    sc_int<16> W_real[W];
    sc_int<16> W_imag[W];
    sc_int<16> tmp_real;
	  sc_int<16> tmp_imag;
	  sc_int<10> len;
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
	  sc_uint<4> stage;
	  sc_uint<4> bits_i;
	  sc_uint<4> bits_index;
	  sc_int<16> result_real, result_imag;
	  sc_int<33> total_temp;
	  sc_int<16> total;
	  int index, index2, windex, incr;

    while (true) {
      index = 0; 
    //Read in the Sample values
      //cout << "Reading in the samples..." << endl;
      
      while( index < 8 )
      {
        //cout << "tmp_real = " << tmp_real << endl;
        tmp_real = i_real.read();
        tmp_imag = i_imag.read();
        
        real[index] = tmp_real;
        imag[index] = tmp_imag;
        index++;
        //wait();
        //cout <<"index = " <<index << endl;
      }
      index = 0;
      cout << "Input received" << endl;
      for (unsigned int a = 0; a < W; ++a) {
        W_real[a] = 0;
			  W_imag[a] = 0;
        wait(CLOCK_PERIOD, SC_NS);
      }

      cout << "Computing TFs" << endl;
	     // Calculate the W-values recursively
	     // <'s'/'u',m,n>: is used in comments to denote a fixed point representation
	     // 's'- signed, 'u'- unsigned, m - no. of integer bits, n - no. of fractional bits
	     //  N = 256
	     //  theta = 8.0*atan(1.0)/N; theta = 1.40625 degree

	     //  w_real =  cos(theta) = 0.9996988187 (00000.11111111111) <s,4,11>
	         w_real =  2047;

	     //  w_imag = -sin(theta) = -0.02454122852 (11111.11111001110) <s,4,11>
	         w_imag = -50;

	     //  w_rec_real = 1(000001.00000000000)
	         w_rec_real = 2048;

	     //  w_rec_real = 0(000000.0000000000)   
	         w_rec_imag = 0;

	    unsigned short w_index;
	    w_index = 0;  
	    for( w_index = 0; w_index < W; ++w_index) 
	    {
	    // <s,9,22> = <s,4,11> * <s,4,11>
	     w_temp1 = w_rec_real*w_real;
       wait(CLOCK_PERIOD, SC_NS);
	     w_temp2 = w_rec_imag*w_imag;
       wait(CLOCK_PERIOD, SC_NS);

	    // <s,9,22> = <s,4,11> * <s,4,11>
	     w_temp3 = w_rec_real*w_imag;
       wait(CLOCK_PERIOD, SC_NS);
	     w_temp4 = w_rec_imag*w_real;
       wait(CLOCK_PERIOD, SC_NS);  

	    // <s,10,22> = <s,9,22> - <s,9,22>
	     w_temp5 = w_temp1 - w_temp2;
       wait(CLOCK_PERIOD, SC_NS);

	    // <s,10,22> = <s,9,22> + <s,9,22>
	     w_temp6 = w_temp3 + w_temp4;
       wait(CLOCK_PERIOD, SC_NS);
	     
	    // assign the sign-bit(MSB)
	     W_real[w_index][15] = w_temp5[32];
	     W_imag[w_index][15] = w_temp6[32];

	    // assign the rest of the bits
	     W_real[w_index].range(14,0) = w_temp5.range(25,11);
	     W_imag[w_index].range(14,0) = w_temp6.range(25,11);

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

        while (stage < M) 
        { 
          len = len >> 1;
        //First Iteration :  Simple calculation, with no multiplies
          int k = 0;
          while(k < N)  
          {
              index =  k; index2 = k + len; 
              tmp_real = real[index] + real[index2];
              wait(CLOCK_PERIOD, SC_NS);
              tmp_imag = imag[index] + imag[index2];
              wait(CLOCK_PERIOD, SC_NS);
              real[index2] = (real[index] - real[index2]);
              wait(CLOCK_PERIOD, SC_NS);
              imag[index2] = (imag[index] - imag[index2]);
              wait(CLOCK_PERIOD, SC_NS);
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
      sc_int<16> real_temp;
      sc_int<16> imag_temp;
      bits_i = 0;
      bits_index = 0;

      //cout << "Writing the transform values..." << endl;
      for ( int r = 0; r < N; ++r)
      {
        bits_i = r;
        bits_index[3]= bits_i[0];
        bits_index[2]= bits_i[1];
        bits_index[1]= bits_i[2];
        bits_index[0]= bits_i[3];
        index = bits_index;
        real_temp = real[index];
        imag_temp = imag[index];
        o_real.write(real_temp); 
        o_imag.write(imag_temp);
        //cout << "real_temp = " << real_temp << endl;
        //cout << "imag_temp = " << imag_temp << endl;
        //cout << "Writing output" << endl;
        //wait();
      }
      index = 0; 
      cout << "FFT done..." << endl;
    }
  }

  void blocking_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay){
    wait(delay);
    // unsigned char *mask_ptr = payload.get_byte_enable_ptr();
    // auto len = payload.get_data_length();
    tlm::tlm_command cmd = payload.get_command();
    sc_dt::uint64 addr = payload.get_address();
    unsigned char *data_ptr = payload.get_data_ptr();

    addr -= base_offset;

    word buffer;
    sc_int<16> in_r;
    sc_int<16> in_i;

    switch (cmd) {
      case tlm::TLM_READ_COMMAND:
        // cout << "READ" << endl;
        switch (addr) {
          case TWOD_FFT_RESULT_ADDR:
            buffer.sint = o_real.read();
            //cout << "Writing result" << endl;
            break;
          case TWOD_FFT_RESULT1_ADDR:
            buffer.sint = o_imag.read();
            //cout << "Writing result" << endl;
            break;
          default:
            std::cerr << "READ Error! twoD_FFT::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
          }
        data_ptr[0] = buffer.character[0];
        data_ptr[1] = buffer.character[1];
        data_ptr[2] = buffer.character[2];
        data_ptr[3] = buffer.character[3];
        break;
      case tlm::TLM_WRITE_COMMAND:
        // cout << "WRITE" << endl;
        switch (addr) {
          case TWOD_FFT_R_ADDR:
            buffer.character[0] = data_ptr[0];
            buffer.character[1] = data_ptr[1];
            buffer.character[2] = data_ptr[2];
            buffer.character[3] = data_ptr[3];
            i_real.write(buffer.sint);
            //cout << "Writing input" << endl;
            break;
          case TWOD_FFT_R1_ADDR:
            buffer.character[0] = data_ptr[0];
            buffer.character[1] = data_ptr[1];
            buffer.character[2] = data_ptr[2];
            buffer.character[3] = data_ptr[3];
            i_imag.write(buffer.sint);
            //cout << "Writing input" << endl;
            break;
          default:
            std::cerr << "WRITE Error! twoD_FFT::blocking_transport: address 0x"
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
};
