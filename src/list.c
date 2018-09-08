struct _list_ {
	void *fst;
	struct _list_ *rst;
};
typedef struct _list_* list;

#define frst rst->fst
#define rrst rst->rst
#define frrst rst->rst->fst
#define frrrst rst->rst->rst->fst
#define frrrrst rst->rst->rst->rst->fst
#define frrrrrst rst->rst->rst->rst->rst->fst
#define rrrrrrst rst->rst->rst->rst->rst->rst

list nil(region reg) {
	list s = region_malloc(reg, sizeof(struct _list_));
	s->fst = s;
	s->rst = NULL;
	return s;
}

bool is_nil(list s) {
	return s->rst == NULL ? true : false;
}

#define foreach(t, u) \
	list _foreach_v = u; \
	for(; !is_nil(_foreach_v) ? (t = _foreach_v->fst, true) : (t = NULL, false); \
		_foreach_v = _foreach_v->rst)


#define foreachlist(w, t, u) \
	w = u; \
	for(; !is_nil(*w) ? (t = (*w)->fst, true) : false; w = &((*w)->rst))

void *append(void *data, list *l, region reg) {
	while(!is_nil(*l)) {
		l = &((*l)->rst);
	}
	*l = region_malloc(reg, sizeof(struct _list_));
	(*l)->fst = data;
	(*l)->rst = nil(reg);
	return &(*l)->fst;
}

list lst(void *data, list l, region reg) {
	list ret = region_malloc(reg, sizeof(struct _list_));
	ret->fst = data;
	ret->rst = l;
	return ret;
}

void append_list(list *fst, list snd) {
	while(!is_nil(*fst)) {
		fst = &((*fst)->rst);
	}
	*fst = snd;
}

void prepend(void *data, list *l, region reg) {
	list ret = region_malloc(reg, sizeof(struct _list_));
	ret->fst = data;
	ret->rst = *l;
	*l = ret;
}

list address_list(list l, region reg) {
	list mapped = nil(reg);
	for(; !is_nil(l); l = l->rst) {
		append(&l->fst, &mapped, reg);
	}
	return mapped;
}

int length(list l) {
	int size;
	for(size = 0; !is_nil(l); l = l->rst, size++);
	return size;
}

list reverse(list l, region reg) {
	list ret = nil(reg);
	void *data;
	foreach(data, l) {
		prepend(data, &ret, reg);
	}
	return ret;
}

list *exists(bool (*pred)(void *, void *), list *l, void *ctx) {
	void *d;
	list *s;
	foreachlist(s, d, l) {
		if(pred(d, ctx)) {
			return s;
		}
	}
	return NULL;
}
