# Data Partitioning of the Gaussian Blur Processing

## The Goal of This HW

In this HW, we're going to add two Gaussian Blur modules to the dual-core riscv-vp platform "tiny32-mc". To achieve parallelism,the image is partitioned into equal parts and a multi-core program is written to issue the processing to the two modules.
