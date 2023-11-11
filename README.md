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
