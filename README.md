# LICO

LICO is a fast lossless image compressor. It takes BMP files (in 24-bit BMP3 format) as input.

To compile serial versions of the compressor and decompressor, use:

```
g++ -O3 -march=native LICO-compressor.cpp -o LICOcompress
g++ -O3 -march=native LICO-decompressor.cpp -o LICOdecompress
```

To compile parallel versions of the compressor and decompressor, use:

```
g++ -O3 -march=native -fopenmp LICO-compressor.cpp -o LICOcompress
g++ -O3 -march=native -fopenmp LICO-decompressor.cpp -o LICOdecompress
```

To compress the file 'image.bmp' into a file named 'image.lico', use:

```
./LICOcompress image.bmp image.lico
```

To decompress the file 'image.lico' into a file named 'decom.bmp', use:

```
./LICOdecompress image.lico decom.bmp
```

Both the compression and decompression take an optional 'y' parameter at the end of the command line that turns on throughput information.

The LICO algorithm is described in detail in the following paper:
* Noushin Azami, Rain Lawson, and Martin Burtscher. "LICO: An Effective, High-Speed, Lossless Compressor for Images." Proceedings of the 2024 Data Compression Conference. Snowbird, UT. March 2024. [[pdf]](https://cs.txstate.edu/~burtscher/papers/dcc24a.pdf)


This code is based upon work supported by the U.S. Department of Energy, Office of Science, Office of Advanced Scientific Research (ASCR), under contract DE-SC0022223.

