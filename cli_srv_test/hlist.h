#ifndef HLIST_H
#define HLIST_H

struct hlist {
    struct hlist *prev;
    struct hlist *next;
};

void hlist_init(struct hlist *list);
void hlist_insert(struct hlist *list, struct hlist *elm);
void hlist_remove(struct hlist *elm);
int hlist_length(const struct hlist *list);
int hlist_empty(const struct hlist *list);
void hlist_insert_list(struct hlist *list, struct hlist *other);

#endif

