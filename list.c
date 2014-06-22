#include "list.h"

List *
list_prepend (List *list, void *data)
{
	List *new;

	new = malloc (sizeof (List));
	new->data = data;
	new->prev = NULL;
	new->next = list;

	if (list)
		list->prev = new;

	return new;
}

List *
list_append (List *list, void *data)
{
	List *it;
	List *new;

	new = malloc (sizeof (List));
	new->data = data;
	new->next = NULL;

	if (!list) {
		list->prev = NULL;
		return new;
	}

	for (it = list; it; it = it->next);

	new->prev = it;

	it->next = new;

	return list;
}

List *
list_reverse (List *list)
{
	List *it;
	List *tmp;

	it = list;

	while (it->next) {
		tmp = it->next;
		it->next = it->prev;
		it->prev = tmp;
	}

	return it;
}

List *
list_remove (List *list, List *remove)
{
	if (remove == NULL)
		return list;

	if (list == remove) {
		list = list->next;
		list->prev = NULL;
	} else {
		remove->prev->next = remove->next;
		remove->next->prev = remove->prev;
	}

	free (remove);

	return list;
}

List *
list_remove_data (List *list, void *data)
{
	List *it;
	for (it = list; it; it = it->next) {
		if (it->data == data)
			return list_remove (list, it);
	}

	return list;
}

