#include "itunesdbi.h"

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#include <stdlib.h>
#include <string.h>

int path_mac_to_unix (char *mac_path, char *unix_dst) {
  int i;

  for (i = 0 ; mac_path[i] != '\0' && i < PATH_MAX ; i++) {
    if (mac_path[i] == ':')
      unix_dst[i] = '/';
    else
      unix_dst[i] = mac_path[i];
  }

  return 0;
}
