#if defined (HAVE_CONFIG_H)
#include "config.h"
#endif

#include <sys/types.h>
#include <stdlib.h>

#include "itunesdbi.h"

#include <machine/endian.h>

#define CRC32POLY 	0x04C11DB7

/*
 * 1024 byte look up table.
 */
void crc32_init_table(void);
static u_int32_t *crc32_table = NULL;

void crc32_init_table(void) {
  u_int32_t i, j, r;
  
  crc32_table = (u_int32_t *)malloc(sizeof(u_int32_t) * 256);
  
  for (i = 0 ; i < 256 ; i++) {
    r = i << 24;
    for (j = 0; j < 8; j++)  {
      if (r & 0x80000000)
	r = (r << 1) ^ CRC32POLY;
      else
	r <<= 1;
    }
    crc32_table[i] = r;
  }
  return;
}

unsigned int crc32 (unsigned char *buf, unsigned int length) {
  unsigned long crc = 0;
  int i;
  
  if (crc32_table == NULL)
    crc32_init_table();
  
  for (i = 0 ; i < length ; i++)
    crc = (crc<<8) ^ crc32_table[((crc >> 24) ^ buf[i]) & 0xff];
  
#if BYTE_ORDER == BIG_ENDIAN
  crc = bswap_32(crc);
#endif /* BIG_ENDIAN */
  
  return crc;
}
