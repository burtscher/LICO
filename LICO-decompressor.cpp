/*
This file is part of LICO, a fast lossless image compressor.

Copyright (c) 2023, Noushin Azami and Martin Burtscher

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

URL: The latest version of this code is available at https://github.com/burtscher/LICO.

Publication: This work is described in detail in the following paper.
Noushin Azami, Rain Lawson, and Martin Burtscher. "LICO: An Effective, High-Speed, Lossless Compressor for Images." Proceedings of the 2024 Data Compression Conference. Snowbird, UT. March 2024.

Sponsor: This code is based upon work supported by the U.S. Department of Energy, Office of Science, Office of Advanced Scientific Research (ASCR), under contract DE-SC0022223.
*/


#define NDEBUG

using byte = unsigned char;

static const int CS = 1024 * 16;  // chunk size (in bytes) [must be multiple of 8]

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sys/time.h>
#include "include/h_BMP_BIT.h"
#include "include/h_ZERE_1.h"
#include "include/h_ZERE_4.h"


struct CPUTimer
{
  timeval beg, end;
  CPUTimer() {}
  ~CPUTimer() {}
  void start() {gettimeofday(&beg, NULL);}
  double stop() {gettimeofday(&end, NULL); return end.tv_sec - beg.tv_sec + (end.tv_usec - beg.tv_usec) / 1000000.0;}
};


static void h_decode(const byte* const __restrict__ input, byte* const __restrict__ output, int& outsize)
{
  // input header
  int* const head_in = (int*)input;
  outsize = head_in[0];

  // initialize
  const int chunks = (outsize + CS - 1) / CS;  // round up
  unsigned short* const size_in = (unsigned short*)&head_in[1];
  byte* const data_in = (byte*)&size_in[chunks];
  int* const start = new int [chunks];

  // convert chunk sizes into starting positions
  int pfs = 0;
  for (int chunkID = 0; chunkID < chunks; chunkID++) {
    start[chunkID] = pfs;
    pfs += (int)size_in[chunkID];
  }

  // process chunks in parallel
  #pragma omp parallel for schedule(dynamic, 1)
  for (int chunkID = 0; chunkID < chunks; chunkID++) {
    // load chunk
    long long chunk1 [CS / sizeof(long long)];
    long long chunk2 [CS / sizeof(long long)];
    byte* in = (byte*)chunk1;
    byte* out = (byte*)chunk2;
    const int base = chunkID * CS;
    const int osize = std::min(CS, outsize - base);
    int csize = size_in[chunkID];
    if (csize == osize) {
      // simply copy
      memcpy(&output[base], &data_in[start[chunkID]], osize);
    } else {
      // decompress
      memcpy(out, &data_in[start[chunkID]], csize);

      // decode
      h_iZERE_1(csize, out, in);
      h_iZERE_4(csize, in, out);

      if (csize != osize) {fprintf(stderr, "ERROR: csize %d does not match osize %d\n\n", csize, osize); exit(-1);}
      memcpy(&output[base], out, csize);
    }
  }

  delete [] start;
}


int main(int argc, char* argv [])
{
  printf("LICO decompressor 1.0\n");
  printf("Copyright 2023 Texas State University\n\n");

  // read input from file
  if (argc < 3) {printf("USAGE: %s compressed_file_name decompressed_file_name [performance_analysis (y)]\n\n", argv[0]);  exit(-1);}

  // read input file
  FILE* const fin = fopen(argv[1], "rb");
  int pre_size = 0;
  const int pre_val = fread(&pre_size, sizeof(pre_size), 1, fin); assert(pre_val == sizeof(pre_size));
  fseek(fin, 0, SEEK_END);
  const int hencsize = ftell(fin);  assert(hencsize > 0);
  byte* const hencoded = new byte [pre_size];
  fseek(fin, 0, SEEK_SET);
  const int insize = fread(hencoded, 1, hencsize, fin);  assert(insize == hencsize);
  fclose(fin);
  printf("encoded size: %d bytes\n", insize);

  // check if third argument is "y" to enable performance analysis
  char* perf_str = argv[3];
  bool perf = false;
  if ((perf_str != nullptr) && (strcmp(perf_str, "y") == 0)) {
    perf = true;
  } else if ((perf_str != nullptr) && (strcmp(perf_str, "y") != 0)) {
    printf("Invalid performance analysis argument. Use 'y' or leave it empty.\n");
    exit(-1);
  }

  // allocate CPU memory
  byte* hdecoded = new byte [pre_size];
  int hdecsize = 0;

  // time CPU decoding
  CPUTimer htimer;
  htimer.start();
  h_decode(hencoded, hdecoded, hdecsize);
  h_iBMP_BIT(hdecsize, hdecoded);
  double hruntime = htimer.stop();

  printf("decoded size: %d bytes\n", hdecsize);
  const float CR = (100.0 * insize) / hdecsize;
  printf("ratio: %6.2f%% %7.3fx\n", CR, 100.0 / CR);
  if (perf) {
    printf("decoding time: %.6f s\n", hruntime);
    double hthroughput = hdecsize * 0.000000001 / hruntime;
    printf("decoding throughput: %8.3f gigabytes/s\n", hthroughput);
  }

  // write to file
  FILE* const fout = fopen(argv[2], "wb");
  fwrite(hdecoded, 1, hdecsize, fout);
  fclose(fout);

  delete [] hencoded;
  delete [] hdecoded;
  return 0;
}
