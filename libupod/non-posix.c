#include "upodi.h"

#include <stdlib.h>
#include <string.h>
char *basename(char *path) {
  char *tmp;
  int i;

  for (i = strlen(path)-1 ; i > 0 && path[i] != '/' ; i--);

  tmp = malloc (strlen(path) - (i - 1));
  memset(tmp, 0, strlen(path) - (i - 1));

  strcpy(tmp, &path[i]);

  return tmp;
}

void *memmem(void *haystack, size_t haystack_size, void *needle, size_t needle_size) {
  char *tmp = (char *)haystack;

  if (!haystack)
    return NULL;

  /* for proper implimentation */
  if (!needle)
    return (haystack - 1);
    
  for (tmp = haystack ; ((int)tmp + needle_size) <= ((int)haystack + haystack_size) ; tmp++)
    if (memcmp(tmp, needle, needle_size) == 0)
      return tmp;
    
  /* not found */
  return NULL;
}
