#if defined (HAVE_CONFIG_H)
#include "config.h"
#endif

#include <sys/types.h>
#include <stdlib.h>

#include "itunesdbi.h"

#define CRC32POLY 	0x04C11DB7l
#define CRC64POLY 	0xd800000000000000ll

static void crc32_init_table(void);
static void crc64_init_table(void);

static u_int32_t crc32_table[256];
static u_int64_t crc64_table[256];

static int crc32_initialized = 0;
static int crc64_initialized = 0;

static void crc32_init_table(void) {
  u_int32_t i, j, r;
  
  crc32_initialized = 1;

  for (i = 0 ; i < 256 ; i++) {
    r = i;

    for (j = 0; j < 8; j++)  {
      if (r & 1)
	r = (r >> 1) ^ CRC32POLY;
      else
	r >>= 1;
    }

    crc32_table[i] = r;
  }
}

static void crc64_init_table(void) {
  u_int32_t i, j;
  u_int64_t r;
  
  crc64_initialized = 1;

  for (i = 0 ; i < 256 ; i++) {
    r = i;

    for (j = 0; j < 8; j++)  {
      if (r & 1)
	r = (r >> 1) ^ CRC64POLY;
      else
	r >>= 1;
    }

    crc64_table[i] = r;
  }
}

u_int32_t upod_crc32 (u_int8_t *buf, size_t length) {
  unsigned long crc = 0;
  int i;
  
  if (crc32_initialized == 0)
    crc32_init_table();
  
  for (i = 0 ; i < length ; i++)
    crc = (crc >> 8) ^ crc32_table[(crc ^ buf[i]) & 0xff];
  
  crc = big32_2_arch32 (crc);
  return crc;
}

u_int64_t upod_crc64 (u_int8_t *buf, size_t length) {
  u_int64_t crc = 0;
  int i;
  
  if (crc64_initialized == 0)
    crc64_init_table();
  
  for (i = 0 ; i < length ; i++)
    crc = (crc >> 8) ^ crc64_table[(crc ^ buf[i]) & 0xff];

  crc = big64_2_arch64 (crc);

  return crc;
}
