#include "itunesdbi.h"
/* this is just for some final testing of implemented featues. it will
   be removed latter on (if even added) */

#include <stdlib.h>

int db_sanity_tree (tree_node_t *node) {
  int i, ret;
  int *iptr = (int *)node->data;
  /* some insanity if there are no children and the children pointer is
     non-null */
  if (node->num_children == 0 && node->children) return -5;

  if ( (iptr[0] == PLHM || iptr[0] == TLHM) && 
       iptr[2] != node->parent->num_children - 1)
    return -1;
  else if (iptr[0] == PYHM && iptr[4] != node->parent->num_children - 2)
    return -3;
  
  if (node->num_children)
    for (i = 0 ; i < node->num_children ; i++) {
      if (node->data == NULL) return -2;

      if (node->children[i]->parent != node) return -20;

      /* check to see if the data is any good */
      if ((iptr[0] == DOHM && node->size != iptr[2]) ||
	  (node->size != iptr[1]))
	  return -45;

      ret = db_sanity_tree (node->children[i]);

      if (ret < 0)
	return ret;
    }

  return 0;
}

int db_sanity_check (itunesdb_t itunesdb) {
  int ret;

  /* ultimate insanity (no db) */
  if (itunesdb.tree_root == NULL) return -100;
  
  /* mildly insane if either song or playlist dont exist */
  if (itunesdb.tree_root->num_children < 2) return -50;

  if ((ret = db_sanity_tree (itunesdb.tree_root)) < 0)
    return ret;

  return 0;
}
