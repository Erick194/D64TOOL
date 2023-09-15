
#ifndef _DECODE_H_
#define _DECODE_H_

int GetOutputSize(void);
int GetReadSize(void);
void DecodeD64(unsigned char* input, unsigned char* output);
unsigned char* EncodeD64Vector(unsigned char* input, int inputlen, int* size);
unsigned char* EncodeD64(unsigned char* input, int inputlen, int* size);

#endif