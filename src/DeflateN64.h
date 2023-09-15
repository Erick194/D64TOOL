#ifndef _D64HACK_DEFLATEN64_H_
#define _D64HACK_DEFLATEN64_H_

void Deflate_Decompress(byte *input, byte *output);
byte *Deflate_Compress(byte *input, int inputlen, int *size);
void InitCountTable();

#endif
