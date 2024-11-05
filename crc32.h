
/* crc32.h */

#include <stdio.h>
#include <malloc.h>

#define CRC32_POLYNOMIAL	0xEDB88320L

int build_crc_table(void);
unsigned long compute_crc32(unsigned char *buffer, int count);
void free_crc_table(void);
