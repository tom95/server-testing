#ifndef __LIST_H__
#define __LIST_H__

#include <stdlib.h>

struct _List;
typedef struct _List List;

struct _List
{
	List *next;
	List *prev;
	void *data;
};

List *
list_append (List *list, void *data);

List *
list_prepend (List *list, void *data);

List *
list_revese (List *list);

List *
list_remove (List *list, List *remove);

List *
list_remove_data (List *list, void *data);

#endif // __LIST_H__

