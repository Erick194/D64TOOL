#ifndef _D64HACK_LZLIB_H_
#define _D64HACK_LZLIB_H_

unsigned char *encode(unsigned char *input, int inputlen, int *size);
void decode(unsigned char *input, unsigned char *output);
int decodedsize(unsigned char *input);
int derror(char *msg);

#endif
