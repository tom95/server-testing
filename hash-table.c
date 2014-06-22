#include "hash-table.h"

struct HashTableData
{
	void *key;
	void *data;
};

static HashTable *
_hash_table_get_iterator (HashTable *self, void *key)
{
	HashTable *it;

	for (it = self; it; it = it->next) {
		if (((struct HashTableData*) it->data)->key == key)
			return it;
	}

	return NULL;
}

HashTable *
hash_table_set (HashTable *self, void *key, void *data)
{
	struct HashTableData *hash;

	hash = malloc (sizeof (struct HashTableData));
	hash->key = key;
	hash->data = data;

	return list_prepend (self, hash);
}

HashTable *
hash_table_remove (HashTable *self, void *key)
{
	HashTable *it;

	it = _hash_table_get_iterator (self, key);
	return list_remove (self, it);
}

void *
hash_table_get (HashTable *self, void *key)
{
	HashTable *it;

	it = _hash_table_get_iterator (self, key);

	if (it)
		return ((struct HashTableData*) it->data)->data;
	else
		return NULL;
}

