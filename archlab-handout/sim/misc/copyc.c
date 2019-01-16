#include <stdio.h>
long copy_block(long *src, long *dest, long len){
  long result = 0;
  while(len > 0){
    long val = *src++;
    *dest++ = val;
    result ^= val;
    len--;
  }
  return result;
}

int main(int argc, char* argv){
  long src[3] = {0x00a, 0x0b0, 0xc00};
  long dest[3] = {0x111, 0x222, 0x333};
  long len = 3;
}
