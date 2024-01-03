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


#ifndef bmp_bit
#define bmp_bit


static inline int h_BMP_BIT_get2(const byte data [])
{
  int ret = data[1];
  ret = (ret << 8) | data[0];
  return ret;
}


static inline int h_BMP_BIT_get4(const byte data [])
{
  int ret = data[3];
  ret = (ret << 8) | data[2];
  ret = (ret << 8) | data[1];
  ret = (ret << 8) | data[0];
  return ret;
}


static inline void h_BMP_BIT_set2(byte data [], const int val)
{
  data[0] = val;
  data[1] = val >> 8;
}


static inline void h_BMP_BIT_set4(byte data [], const int val)
{
  data[0] = val;
  data[1] = val >> 8;
  data[2] = val >> 16;
  data[3] = val >> 24;
}


static inline void h_BMP_BIT(int& size, byte*& data)
{
  assert(sizeof(unsigned long long) == 8);

  if (size < 54) {
    printf("h_BMP_BIT: WARNING file size is too small for a BMP image\n");
  } else {
    const int w = h_BMP_BIT_get4(&data[18]);
    const int h = h_BMP_BIT_get4(&data[22]);
    const int pad = ((w * 3 + 3) & ~3) - (w * 3);
    const int width = w * 3 + pad;
    if ((data[0] != 'B') || (data[1] != 'M') || (h_BMP_BIT_get4(&data[2]) != 54 + h * width) || (h_BMP_BIT_get4(&data[10]) != 54) || (h_BMP_BIT_get4(&data[14]) != 40) || (h_BMP_BIT_get2(&data[26]) != 1) || (h_BMP_BIT_get2(&data[28]) != 24) || (h_BMP_BIT_get4(&data[30]) != 0) || (h_BMP_BIT_get4(&data[34]) != h * width) || (h_BMP_BIT_get4(&data[46]) != 0) || (h_BMP_BIT_get4(&data[50]) != 0) || (size != 54 + h * width) || (w < 1) || (h < 1)) {
      printf("h_BMP_BIT: WARNING: not a supported BMP format\n");
    } else {
      data[0] = data[0] - 'B';  // B
      data[1] = data[1] - 'M';  // M
      h_BMP_BIT_set4(&data[2], h_BMP_BIT_get4(&data[2]) - (h * width + 54));  // size in bytes
      //h_BMP_BIT_set4(&data[6], h_BMP_BIT_get4(&data[6]));  // 2 reserved values (0, 0)
      h_BMP_BIT_set4(&data[10], h_BMP_BIT_get4(&data[10]) - 54);  // offset to image data (54)
      h_BMP_BIT_set4(&data[14], h_BMP_BIT_get4(&data[14]) - 40);  // header size (40)
      //h_BMP_BIT_set4(&data[18], w);  // width
      //h_BMP_BIT_set4(&data[22], h);  // height
      h_BMP_BIT_set2(&data[26], h_BMP_BIT_get2(&data[26]) - 1);  // color planes (must be 1)
      h_BMP_BIT_set2(&data[28], h_BMP_BIT_get2(&data[28]) - 24);  // bits per pixel (only 24 supported)
      //h_BMP_BIT_set4(&data[30], h_BMP_BIT_get4(&data[30]) - 0);  // compression method (only 0 supported)
      h_BMP_BIT_set4(&data[34], h_BMP_BIT_get4(&data[34]) - (h * width));  // image size
      //h_BMP_BIT_set4(&data[38], h_BMP_BIT_get4(&data[38]));  // horizontal resolution
      h_BMP_BIT_set4(&data[42], h_BMP_BIT_get4(&data[42]) - h_BMP_BIT_get4(&data[38]));  // vertical resolution [same as previous?]
      //h_BMP_BIT_set4(&data[46], h_BMP_BIT_get4(&data[46]) - 0);  // number of colors or 0
      //h_BMP_BIT_set4(&data[50], h_BMP_BIT_get4(&data[50]) - 0);  // important colors or 0

      const int newsize = w * h * sizeof(byte);
      unsigned long long* const temp = new unsigned long long [(newsize * 3 + 7) / 8];
      byte* const tmp = (byte*)temp;
      byte* const bmp = (byte*)&data[54];

      // encode
      #pragma omp parallel for default(none) shared(width, w, h, bmp, tmp, newsize)
      for (int y = 0; y < h; y++) {
        int p0 = 0, p1 = 0, p2 = 0;
        if (y > 0) {  // pixel y DIFF
          p0 = bmp[(y - 1) * width + 0];
          p1 = bmp[(y - 1) * width + 1];
          p2 = bmp[(y - 1) * width + 2];
        }
        for (int x = 0; x < w; x++) {
          // read and split into color channels
          const int n0 = bmp[y * width + x * 3 + 0];
          const int n1 = bmp[y * width + x * 3 + 1];
          const int n2 = bmp[y * width + x * 3 + 2];

          // pixel x DIFF
          int v0 = n0 - p0;
          int v1 = n1 - p1;
          int v2 = n2 - p2;
          p0 = n0;
          p1 = n1;
          p2 = n2;

          // color-channel DIFF
          v0 -= v1;
          v2 -= v1;

          // TCMS
          v0 = ((v0 << 1) ^ ((v0 << 24) >> 31)) & 0xff;
          v1 = ((v1 << 1) ^ ((v1 << 24) >> 31)) & 0xff;
          v2 = ((v2 << 1) ^ ((v2 << 24) >> 31)) & 0xff;

          // transpose, separate channels TUPL3, write encoded values
          tmp[0 * newsize + y + x * h] = v0;
          tmp[1 * newsize + y + x * h] = v1;
          tmp[2 * newsize + y + x * h] = v2;
        }
      }

      // BIT_1
      const int csize = newsize * 3;
      const int extra = csize % (8 * 8 / 8);
      const int esize = csize - extra;
      #pragma omp parallel for default(none) shared(esize, bmp, temp)
      for (int pos = 0; pos < esize; pos += 8) {
        unsigned long long t, x;
        x = temp[pos / 8];
        t = (x ^ (x >> 7)) & 0x00AA00AA00AA00AAULL;
        x = x ^ t ^ (t << 7);
        t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCULL;
        x = x ^ t ^ (t << 14);
        t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0ULL;
        x = x ^ t ^ (t << 28);
        for (int i = 0; i < 8; i++) {
          bmp[pos / 8 + i * (esize / 8)] = x >> (i * 8);
        }
      }

      // copy leftover bytes
      for (int i = 0; i < extra; i++) {
        bmp[csize - extra + i] = tmp[csize - extra + i];
      }

      // handle padding (if any)
      for (int i = newsize * 3; i < width * h; i++) {
        bmp[i] = 0;
      }

      delete [] temp;
    }
  }
}


static inline void h_iBMP_BIT(int& size, byte*& data)
{
  assert(sizeof(unsigned long long) == 8);

  if (size < 54) {
    printf("h_BMP_BIT: WARNING file size is too small for a BMP image\n");
  } else {
    const int w = h_BMP_BIT_get4(&data[18]);
    const int h = h_BMP_BIT_get4(&data[22]);
    const int pad = ((w * 3 + 3) & ~3) - (w * 3);
    const int width = w * 3 + pad;
    if ((data[0] != 0) || (data[1] != 0) || (h_BMP_BIT_get4(&data[2]) != 0) || (h_BMP_BIT_get4(&data[10]) != 0) || (h_BMP_BIT_get4(&data[14]) != 0) || (h_BMP_BIT_get2(&data[26]) != 0) || (h_BMP_BIT_get2(&data[28]) != 0) || (h_BMP_BIT_get4(&data[30]) != 0) || (h_BMP_BIT_get4(&data[34]) != 0) || (h_BMP_BIT_get4(&data[46]) != 0) || (h_BMP_BIT_get4(&data[50]) != 0) || (w < 1) || (h < 1)) {
      printf("h_BMP_BIT: WARNING not a supported BMP format\n");
    } else {
      data[0] = data[0] + 'B';  // B
      data[1] = data[1] + 'M';  // M
      h_BMP_BIT_set4(&data[2], h_BMP_BIT_get4(&data[2]) + (h * width + 54));  // size in bytes
      //h_BMP_BIT_set4(&data[6], h_BMP_BIT_get4(&data[6]));  // 2 reserved values (0, 0)
      h_BMP_BIT_set4(&data[10], h_BMP_BIT_get4(&data[10]) + 54);  // offset to image data (54)
      h_BMP_BIT_set4(&data[14], h_BMP_BIT_get4(&data[14]) + 40);  // header size (40)
      //h_BMP_BIT_set4(&data[18], w);  // width
      //h_BMP_BIT_set4(&data[22], h);  // height
      h_BMP_BIT_set2(&data[26], h_BMP_BIT_get2(&data[26]) + 1);  // color planes (must be 1)
      h_BMP_BIT_set2(&data[28], h_BMP_BIT_get2(&data[28]) + 24);  // bits per pixel (only 24 supported)
      //h_BMP_BIT_set4(&data[30], h_BMP_BIT_get4(&data[30]) + 0);  // compression method (only 0 supported)
      h_BMP_BIT_set4(&data[34], h_BMP_BIT_get4(&data[34]) + (h * width));  // image size
      //h_BMP_BIT_set4(&data[38], h_BMP_BIT_get4(&data[38]));  // horizontal resolution
      h_BMP_BIT_set4(&data[42], h_BMP_BIT_get4(&data[42]) + h_BMP_BIT_get4(&data[38]));  // vertical resolution [same as previous?]
      //h_BMP_BIT_set4(&data[46], h_BMP_BIT_get4(&data[46]) + 0);  // number of colors or 0
      //h_BMP_BIT_set4(&data[50], h_BMP_BIT_get4(&data[50]) + 0);  // important colors or 0

      const int newsize = w * h * sizeof(byte);
      unsigned long long* const temp = new unsigned long long [(newsize * 3 + 7) / 8];
      byte* const tmp = (byte*)temp;
      byte* const bmp = (byte*)&data[54];

      // iBIT_1
      const int csize = newsize * 3;
      const int extra = csize % (8 * 8 / 8);
      const int esize = csize - extra;
      #pragma omp parallel for default(none) shared(esize, bmp, temp)
      for (int pos = 0; pos < esize; pos += 8) {
        unsigned long long t, x = 0;
        for (int i = 0; i < 8; i++) x |= (unsigned long long)bmp[pos / 8 + i * (esize / 8)] << (i * 8);
        t = (x ^ (x >> 7)) & 0x00AA00AA00AA00AAULL;
        x = x ^ t ^ (t << 7);
        t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCULL;
        x = x ^ t ^ (t << 14);
        t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0ULL;
        x = x ^ t ^ (t << 28);
        temp[pos / 8] = x;
      }

      // copy leftover bytes
      for (int i = 0; i < extra; i++) {
        tmp[csize - extra + i] = bmp[csize - extra + i];
      }

      // decode
      int prev0 = 0, prev1 = 0, prev2 = 0;
      for (int y = 0; y < h; y++) {
        // read values, combine channels iTUPL3, inverse transpose
        int v0 = tmp[0 * newsize + y];
        int v1 = tmp[1 * newsize + y];
        int v2 = tmp[2 * newsize + y];

        // inverse TCMS
        v0 = (v0 >> 1) ^ ((v0 << 31) >> 31);
        v1 = (v1 >> 1) ^ ((v1 << 31) >> 31);
        v2 = (v2 >> 1) ^ ((v2 << 31) >> 31);

        // inverse color-channel DIFF
        v0 += v1;
        v2 += v1;

        // inverse pixel y DIFF
        v0 += prev0;
        v1 += prev1;
        v2 += prev2;

        // write decoded values
        bmp[y * width + 0] = prev0 = v0;
        bmp[y * width + 1] = prev1 = v1;
        bmp[y * width + 2] = prev2 = v2;
      }

      #pragma omp parallel for default(none) shared(width, w, h, tmp, bmp, newsize)
      for (int y = 0; y < h; y++) {
        int prev0 = bmp[y * width + 0];
        int prev1 = bmp[y * width + 1];
        int prev2 = bmp[y * width + 2];
        for (int x = 1; x < w; x++) {
          // read values, combine channels iTUPL3, inverse transpose
          int v0 = tmp[0 * newsize + y + x * h];
          int v1 = tmp[1 * newsize + y + x * h];
          int v2 = tmp[2 * newsize + y + x * h];

          // inverse TCMS
          v0 = (v0 >> 1) ^ ((v0 << 31) >> 31);
          v1 = (v1 >> 1) ^ ((v1 << 31) >> 31);
          v2 = (v2 >> 1) ^ ((v2 << 31) >> 31);

          // inverse color-channel DIFF
          v0 += v1;
          v2 += v1;

          // inverse pixel x DIFF
          v0 += prev0;
          v1 += prev1;
          v2 += prev2;

          // write decoded values
          bmp[y * width + x * 3 + 0] = prev0 = v0;
          bmp[y * width + x * 3 + 1] = prev1 = v1;
          bmp[y * width + x * 3 + 2] = prev2 = v2;
        }
      }

      // set padding bytes (if any) to zero
      if (pad > 0) {
        for (int y = 0; y < h; y++) {
          for (int x = w * 3; x < width; x++) {
            bmp[y * width + x] = 0;
          }
        }
      }

      delete [] temp;
    }
  }
}


#endif
