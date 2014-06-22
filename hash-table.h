#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include <stdlib.h>

#include "list.h"

typedef List HashTable;

HashTable *
hash_table_set (HashTable *self, void *key, void *data);

HashTable *
hash_table_remove (HashTable *self, void *key);

void *
hash_table_get (HashTable *self, void *key);

#endif // __HASH_TABLE_H__

