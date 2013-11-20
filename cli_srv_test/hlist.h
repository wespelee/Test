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

#define container_of(ptr, sample, member)				\
	(__typeof__(sample))((char *)(ptr)	-			\
		 ((char *)&(sample)->member - (char *)(sample)))

#define hlist_for_each(pos, head, member)				\
	for (pos = 0, pos = container_of((head)->next, pos, member);	\
	     &pos->member != (head);					\
	     pos = container_of(pos->member.next, pos, member))

#define hlist_for_each_safe(pos, tmp, head, member)			\
	for (pos = 0, tmp = 0, 						\
	     pos = container_of((head)->next, pos, member),		\
	     tmp = container_of((pos)->member.next, tmp, member);	\
	     &pos->member != (head);					\
	     pos = tmp,							\
	     tmp = container_of(pos->member.next, tmp, member))

#define hlist_for_each_reverse(pos, head, member)			\
	for (pos = 0, pos = container_of((head)->prev, pos, member);	\
	     &pos->member != (head);					\
	     pos = container_of(pos->member.prev, pos, member))

#define hlist_for_each_reverse_safe(pos, tmp, head, member)		\
	for (pos = 0, tmp = 0, 						\
	     pos = container_of((head)->prev, pos, member),	\
	     tmp = container_of((pos)->member.prev, tmp, member);	\
	     &pos->member != (head);					\
	     pos = tmp,							\
	     tmp = container_of(pos->member.prev, tmp, member))

#endif

