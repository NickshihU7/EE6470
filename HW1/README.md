# Gaussian Blur with SystemC

## The Goal of This HW

We're gping to perform a 3x3 Gaussian blur on an input bitmap with SystemC.   
The SystemC module contains the following three process:

- An Input process to parse image format and send pixels to next process
- A Computation process to receive pixels and do Gaussian blur. Then pass results to the next process.
- An Output process to dump processed pixels as a BMP image.
