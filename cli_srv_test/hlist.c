#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"
#include "hlist.h"

SRV_EXPORT void
hlist_init(struct hlist *list)
{
	list->prev = list;
	list->next = list;
}

SRV_EXPORT void
hlist_insert(struct hlist *list, struct hlist *elm)
{
	elm->prev = list;
	elm->next = list->next;
	list->next = elm;
	elm->next->prev = elm;
}

SRV_EXPORT void
hlist_remove(struct hlist *elm)
{
	elm->prev->next = elm->next;
	elm->next->prev = elm->prev;
	elm->next = NULL;
	elm->prev = NULL;
}

SRV_EXPORT int
hlist_length(const struct hlist *list)
{
	struct hlist *e;
	int count;

	count = 0;
	e = list->next;
	while (e != list) {
		e = e->next;
		count++;
	}

	return count;
}

SRV_EXPORT int
hlist_empty(const struct hlist *list)
{
	return list->next == list;
}

SRV_EXPORT void
hlist_insert_list(struct hlist *list, struct hlist *other)
{
	if (hlist_empty(other))
		return;

	other->next->prev = list;
	other->prev->next = list->next;
	list->next->prev = other->prev;
	list->next = other->next;
}
