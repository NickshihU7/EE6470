# Gaussian Blur with FIFO channels and row buffers

The goal is to implement a Gaussian blur filter with SystemC modules connected with SystemC FIFO channels. In addition, we'd like to send in batch a row or a column of image pixels from Input to Calculation.

## System Architecture

The system architecture uses the TLM 2.0 sockets, including initiator and target sockets, to realize point-to-point connection between modules as shown in the figure below. The module `Testbench` works as TLM initiators, and the `GaussianBlur` plays the role of the TLM target.

<div align="center"> <img src="hw2.png" width="60%"/> </div>

## Implementation

The TLM socket biding between the two modules `Testbench` and `GaussianBlur` is implemented as follows:

    Testbench tb("tb");
    GaussianBlur gaussian_filter("gaussian_filter");
    tb.initiator.i_skt(gaussian_filter.t_skt);

In Testbench's implementation, we initiate write transactions through the initiator socket in Initiator module to put the pixels to be processed by GaussianBlur, and then initiator read transactions to get the results. The correspoding codes are written in `Teatbench.cpp` as follows.

    for (y = 0; y != height; ++y) {
        for (x = 0; x != width; ++x) {
            adjustX = (filterWidth % 2) ? 1 : 0; // 1
            adjustY = (filterHeight % 2) ? 1 : 0; // 1
            xBound = filterWidth / 2;            // 1
            yBound = filterHeight / 2;            // 1

            for (v = -yBound; v != yBound + adjustY; ++v) {   //-1, 0, 1
                for (u = -xBound; u != xBound + adjustX; ++u) { //-1, 0, 1
                if (x + u >= 0 && x + u < width && y + v >= 0 && y + v < height) {
                    R = *(source_bitmap +
                        bytes_per_pixel * (width * (y + v) + (x + u)) + 2);
                    G = *(source_bitmap +
                        bytes_per_pixel * (width * (y + v) + (x + u)) + 1);
                    B = *(source_bitmap +
                        bytes_per_pixel * (width * (y + v) + (x + u)) + 0);
                } else {
                    R = 0;
                    G = 0;
                    B = 0;
                }
                data.uc[0] = R;
                data.uc[1] = G;
                data.uc[2] = B;
                mask[0] = 0xff;
                mask[1] = 0xff;
                mask[2] = 0xff;
                mask[3] = 0;
                initiator.write_to_socket(GAUSSIAN_FILTER_R_ADDR, mask, data.uc, 4);
                }
            }

            initiator.read_from_socket(GAUSSIAN_FILTER_RESULT_ADDR1, mask, data.uc, 4);
            red = data.uint;
            initiator.read_from_socket(GAUSSIAN_FILTER_RESULT_ADDR2, mask, data.uc, 4);
            green = data.uint;
            initiator.read_from_socket(GAUSSIAN_FILTER_RESULT_ADDR3, mask, data.uc, 4);
            blue = data.uint;

            //truncate values smaller than zero and larger than 255
            *(target_bitmap + bytes_per_pixel * (width * y + x) + 2) = min(max(int(factor * red + bias), 0), 255);
            *(target_bitmap + bytes_per_pixel * (width * y + x) + 1) = min(max(int(factor * green + bias), 0), 255);
            *(target_bitmap + bytes_per_pixel * (width * y + x) + 0) = min(max(int(factor * blue + bias), 0), 255);
        }
    }

## How to execute the codes

1.  Compile the program

        $ mkdir build
        $ cd build
        $ cmake ..
        $ make

2.  Run the model program

        $ make run

## Result

|Input Bitmap | Output Bitmap|
|---------------|---------------|
|![i](lena_std_short.bmp)|![o](out.bmp)|


## Conclusion

From this HW I learnt how to use TLM 2.0 sockets to manage the point-to-point connection between sc_modules.
