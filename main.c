#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

/*
 * Temporary Buffer
 */
#define TEMP_BUFFER_SIZE 8192
static struct {
	char buffer[TEMP_BUFFER_SIZE];
	char* curr;
	char* marker;
} __temp = { 0 };
void* temp_buffer_alloc(size_t n_bytes) {
	if (!__temp.curr) { __temp.curr = __temp.buffer; }
	if ((size_t)__temp.curr + n_bytes > (size_t)__temp.buffer + TEMP_BUFFER_SIZE) { return NULL; }
	void* ret = __temp.curr;
	__temp.curr += n_bytes;
	return ret;
}
#define temp_buffer_reset() do { __temp.curr = __temp.buffer; } while(0)
#define temp_buffer_marker_set() do { __temp.marker = __temp.curr; } while (0)
#define temp_buffer_marker_reset() do { __temp.curr = __temp.marker; } while(0)

/*
 * Array
 */
typedef struct {
	size_t size;
	size_t len;
	size_t cap;

	char is_ptr;
	void (*defer_fn)(void*);
	int (*cmp_fn)(const void*, const void*);
	void (*print_fn)(const void*);
} __ArrayHeader;

#define ARRAY_HEADER_SIZE (sizeof(__ArrayHeader))
#define ARRAY_BASE_SIZE 16
#define ARRAY_GET_HEADER(arr) ((__ArrayHeader*)(arr) - 1)

#define array_length(arr) (ARRAY_GET_HEADER(arr)->len)
#define array_reset(arr) do { ARRAY_GET_HEADER(arr)->len = 0; } while(0)

struct __ArrayParams {
	size_t __size;
	size_t reserve;
	char is_ptr;

	void (*defer_fn)(void*);
	int (*cmp_fn)(const void*, const void*);
	void (*print_fn)(const void*);
};
void* __array_new(struct __ArrayParams params) {
	__ArrayHeader* arr;
	if (!params.reserve) {
		arr = malloc(ARRAY_HEADER_SIZE + ARRAY_BASE_SIZE * params.__size);
		if (!arr) { return NULL; }
		arr->cap = ARRAY_BASE_SIZE;
	} else {
		arr = malloc(ARRAY_HEADER_SIZE + params.reserve * params.__size);
		if (!arr) { return NULL; }
		arr->cap = params.reserve;
	}
	arr->size = params.__size;
	arr->len = 0;
	arr->is_ptr = params.is_ptr;
	arr->print_fn = params.print_fn;
	arr->cmp_fn = params.cmp_fn;
	arr->defer_fn = params.defer_fn;
	return (void*)(arr + 1);
}
#define __array_new_params(...) __array_new((struct __ArrayParams){ 0, __VA_ARGS__ })
#define array_new(T, ...) (T*)__array_new_params(.__size = sizeof(T), __VA_ARGS__)
void* array_copy(void* arr) {
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);

	void* res = malloc(ARRAY_HEADER_SIZE + head->cap * head->size);
	if (!res) { return NULL; }
	__ArrayHeader* res_head = res;
	memcpy(res_head, head, ARRAY_HEADER_SIZE + head->len * head->size);

	return (void*)(res_head + 1);
}
void __array_expand(void** arr_ptr, size_t n) {
	assert(*arr_ptr);

	__ArrayHeader* head = ARRAY_GET_HEADER(*arr_ptr);
	size_t old_cap = head->cap;
	while (n + head->len > head->cap) { head->cap <<= 1; }
	void* new_arr = malloc(ARRAY_HEADER_SIZE + head->cap * head->size);
	memcpy(new_arr, head, ARRAY_HEADER_SIZE + old_cap * head->size);
	free(head);

	*arr_ptr = (__ArrayHeader*)new_arr + 1;
}
void __array_append(void** arr, void* val) {
	assert(*arr);
	__ArrayHeader* head = ARRAY_GET_HEADER(*arr);
	if (head->len == head->cap) {
		__array_expand(arr, 1);
		head = ARRAY_GET_HEADER(*arr);
	}
	memcpy((char*)(*arr) + head->len++ * head->size, val, head->size);
}
#define array_append(arr, val) do { \
	typeof(*arr) v = (val); \
	__array_append((void**)&arr, &v); \
} while(0)
#define __swap(a, b, size) do { \
	temp_buffer_marker_set(); \
		void* temp = temp_buffer_alloc(size); \
		assert(temp && "ERROR: Can not allocate to the temporary buffer anymore!"); \
		memcpy(temp, a, size); \
		memcpy(a, b, size); \
		memcpy(b, temp, size); \
	temp_buffer_marker_reset(); \
} while(0);
void* __array_remove(void* arr, int pos) {
	assert(arr);
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	if (pos < 0) { pos += head->len; }
	assert(pos < (int)head->len);
	--head->len;
	__swap((void*)((char*)arr + pos * head->size), (void*)((char*)arr + head->len * head->size), head->size);
	return (void*)((char*)arr + head->len * head->size);
}
#define array_remove(arr, pos) *(typeof(*arr)*)__array_remove((void*)arr, pos)
#define array_pop(arr) *(typeof(*arr)*)__array_remove((void*)arr, -1)
void* __array_at(void* arr, int pos) {
	if (!arr) { return NULL; }

	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	if (pos < 0) { pos += head->len; }
	if (pos >= (int)head->len) { return NULL; }

	return (char*)arr + pos * head->size;
}
#define array_at(arr, pos) (typeof(*arr)*)__array_at(arr, pos)
void array_free(void* arr) {
	assert(arr);

	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	if (head->defer_fn) {
		if (!head->is_ptr) {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn((char*)arr + i * head->size);
			}
		} else {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn(*(void**)((char*)arr + i * head->size));
			}
		}
	}
	free(head);
}

#define array_foreach(val, arr) for ( struct { typeof(*arr)* value; size_t idx; } val = { .value = arr, .idx = 0 }; val.idx < array_length(arr); ++val.idx, ++val.value )
void array_set_cmp(void* arr, int (*cmp)(const void*, const void*)) { 
	assert(arr);

	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	head->cmp_fn = cmp;
}
void array_print(const void* arr) {
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	assert(head->print_fn && "ERROR: No print function provided to the array!");
	printf("{ ");
	if (!head->is_ptr) {
		for (size_t i = 0; i < head->len; ++i) {
			if (i != 0) { printf(", "); }
			head->print_fn((char*)arr + i * head->size);
		}
	} else {
		for (size_t i = 0; i < head->len; ++i) {
			if (i != 0) { printf(", "); }
			head->print_fn(*(void**)((char*)arr + i * head->size));
		}
	}
	printf(" }");
}
void __array_sort(void* arr, int start, int end) {
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	if (end - start <= 1 || end < start) {
		if (end - start == 1 && (!head->is_ptr ?
			head->cmp_fn((char*)arr + start * head->size, (char*)arr + (start + 1) * head->size) == 1 :
			head->cmp_fn(*(void**)((char*)arr + start * head->size), *(void**)((char*)arr + (start + 1) * head->size)) == 1)) {
			__swap((char*)arr + start * head->size, (char*)arr + (start + 1) * head->size, head->size);
		}
		return;
	}
	void* pivot = (char*)arr + end * head->size;
	void* a = (char*)arr + (start - 1) * head->size;
	if (!head->is_ptr) {
		for (void* b = (char*)arr + start * head->size; b < pivot; b = (char*)b + head->size) {
			if (head->cmp_fn(b, pivot) == -1) {
				a = (char*)a + head->size;
				__swap(a, b, head->size);
			}
		}
	} else {
		for (void* b = (char*)arr + start * head->size; b < pivot; b = (char*)b + head->size) {
			if (head->cmp_fn(*(void**)b, *(void**)pivot) == -1) {
				a = (char*)a + head->size;
				__swap(a, b, head->size);
			}
		}
	}
	a = (char*)a + head->size;
	int mid = ((size_t)a - (size_t)arr) / head->size;
	__swap(a, pivot, head->size);

	__array_sort(arr, start, mid - 1);
	__array_sort(arr, mid + 1, end);
}
#define array_sort(arr) do { \
	assert(arr); \
	assert(ARRAY_GET_HEADER(arr)->cmp_fn && "ERROR: Trying to sort without a compare function provided!"); \
	__array_sort(arr, 0, ARRAY_GET_HEADER(arr)->len - 1); \
} while(0)
void* __array_map(const void* const arr, void (*fn)(void*, const void*)) {
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);

	void* curr = (void*)arr;
	void* res = __array_new((struct __ArrayParams){
		.__size = head->size,
		.reserve = head->cap,
		.defer_fn = head->defer_fn,
		.print_fn = head->print_fn,
		.cmp_fn = head->cmp_fn,
	});

	temp_buffer_marker_set();
	void* curr_res = temp_buffer_alloc(head->size);
	for (size_t i = 0; i < head->len; ++i) {
		fn(curr_res, curr);
		__array_append(&res, curr_res);
		curr = (char*)curr + head->size;
	}
	temp_buffer_marker_reset();
	return res;
}
#define array_map(arr, fn) (typeof(*arr)*)__array_map(arr, fn)
void* array_map_self(void* arr, void (*fn)(void*, const void*)) {
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	void* val = arr;

	temp_buffer_marker_set();
	void* res = temp_buffer_alloc(head->size);
	for (size_t i = 0; i < head->len; ++i) {
		fn(res, val);
		memcpy(val, res, head->size);
		val = (char*)val + head->size;
	}
	temp_buffer_marker_reset();
	return arr;
}
void* __array_filter(const void* const arr, int (*fn)(const void*)) {
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);

	void* curr = (void*)arr;
	void* res = __array_new((struct __ArrayParams){
		.__size = head->size,
		.is_ptr = head->is_ptr,
		.reserve = head->cap,
		.defer_fn = head->defer_fn,
		.print_fn = head->print_fn,
		.cmp_fn = head->cmp_fn,
	});

	for (size_t i = 0; i < head->len; ++i) {
		if (fn(curr)) { __array_append(&res, curr); }
		curr = (char*)curr + head->size;
	}
	return res;
}
#define array_filter(arr, fn) (typeof(*arr)*)__array_filter(arr, fn)
void* array_filter_self(void* arr, int (*fn)(const void*)) {
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	size_t move_back = 0;
	
	for (size_t i = 0; i < head->len; ++i) {
		if (fn((char*)arr + i * head->size)) {
			memcpy((char*)arr + (i - move_back) * head->size, (char*)arr + i * head->size, head->size);
		} else {
			++move_back;
		}
	}
	head->len -= move_back;
	return arr;
}
int array_find(void* arr, void* val) {
	assert(arr);
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	for (size_t i = 0; i < head->len; ++i) {
		if (!head->cmp_fn(val, (char*)arr + i * head->size)) {
			return (int)i;
		}
	}
	return -1;
}
int array_find_by(void* arr, int (*fn)(const void*)) {
	assert(arr);
	__ArrayHeader* head = ARRAY_GET_HEADER(arr);
	for (size_t i = 0; i < head->len; ++i) {
		if (fn((char*)arr + i * head->size)) {
			return (int)i;
		}
	}
	return -1;
}

/*
 * String
 */

typedef struct {
	size_t len;
	size_t cap;
} __StringHeader;

#define STRING_BASE_SIZE 16
#define STRING_HEADER_SIZE (sizeof(__StringHeader))
#define STRING_GET_HEADER(str) ((__StringHeader*)str - 1)

#define str_length(str) (STRING_GET_HEADER(str)->len)
#define STR_FMT "%.*s"
#define STR_UNPACK(str) (int)str_length(str), str

char* str_new() {
	__StringHeader* head = malloc(STRING_HEADER_SIZE + STRING_BASE_SIZE * sizeof(char));
	assert(head);
	memset(head, 0, STRING_HEADER_SIZE + STRING_BASE_SIZE * sizeof(char));
	head->cap = STRING_BASE_SIZE;
	char* str = (char*)(head + 1);
	return str;
}
char* str_from_lit_n(const char* c_str, size_t n) {
	__StringHeader* head = malloc(STRING_HEADER_SIZE + n * sizeof(char));
	memcpy(head + 1, c_str, n * sizeof(char));
	head->cap = n;
	head->len = n;
	return (char*)(head + 1);
}
#define str_from_lit(c_str) str_from_lit_n(c_str, strlen(c_str))
#define str_copy(str) str_from_lit_n(str, STRING_GET_HEADER(str)->len)
void str_print(const void* str) {
	printf("\""STR_FMT"\"", STR_UNPACK((char*)str));
}
void str_free(void* str) {
	assert(str);
	free(STRING_GET_HEADER((void*)str));
}

int str_eq(const void* val1, const void* val2) {
	char* str1 = (char*)val1;
	char* str2 = (char*)val2;
	if (str_length(str1) != str_length(str2)) { return 0; }
	for (size_t i = 0; i < str_length(str1); ++i) {
		if (str1[i] != str2[i]) { return 0; }
	}
	return 1;
}
int str_cmp(const void* val1, const void* val2) {
	char* str1 = (char*)val1;
	char* str2 = (char*)val2;
	int ret = strncmp(str1, str2, str_length(str1) < str_length(str2) ? str_length(str1) : str_length(str2)); 
	if (ret < 0) return -1;
	else if (ret > 0) return 1;
	else return 0;
}

/*
 * Bitmap
 */
#define bitmap_size(bitmap) (*(bitmap - 1))
size_t* bitmap_new(size_t size) {
	size_t* ret = calloc(2 + (size - 1) / (8 * sizeof(size_t)), sizeof(size_t));
	if (!ret) { return NULL; }
	*ret = size;
	return ret + 1;
}
size_t* bitmap_copy(const size_t* const bitmap) {
	size_t* ret = malloc(((bitmap_size(bitmap) - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t));
	if (!ret) { return NULL; }
	*ret = bitmap_size(bitmap);
	memcpy(ret + 1, bitmap, (((*ret - 1) / (8 * sizeof(size_t)) + 1) * sizeof(size_t)));
	return ret + 1;
}
int bitmap_get(const size_t* const bitmap, int pos) {
	assert(bitmap);
	if (pos < 0) { pos += bitmap_size(bitmap); }
	assert(pos < (int)bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);
	return bitmap[bitmap_pos] & (0b1ull << inbyte_pos) ? 1 : 0;
}
void bitmap_flip(size_t* bitmap, int pos) {
	assert(bitmap);
	if (pos < 0) { pos += bitmap_size(bitmap); }
	assert(pos < (int)bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);
	if (bitmap[bitmap_pos] & (0b1 << inbyte_pos) ? 1 : 0) {
		bitmap[bitmap_pos] |= 0b1 << inbyte_pos;
	} else {
		bitmap[bitmap_pos] &= ~(0b1 << inbyte_pos);
	}
}
void bitmap_set(size_t* bitmap, int pos, int val) {
	assert(bitmap);
	if (pos < 0) { pos += bitmap_size(bitmap); }
	assert(pos < (int)bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);
	if (val) {
		bitmap[bitmap_pos] |= 0b1ull << inbyte_pos;
	} else {
		bitmap[bitmap_pos] &= ~(0b1ull << inbyte_pos);
	}
}
void bitmap_free(size_t* bitmap) {
	assert(bitmap);
	free(bitmap - 1);
}
void bitmap_print(size_t* bitmap) {
	printf("0b");
	for (size_t i = 0; i < bitmap_size(bitmap); ++i) {
		printf("%c", bitmap_get(bitmap, i) + '0');
	}
}

size_t* bitmap_and(const size_t* const bitmap1, const size_t* const bitmap2) {
	if (!bitmap1 || !bitmap2 || bitmap_size(bitmap1) != bitmap_size(bitmap2)) { return NULL; }
	size_t* res = bitmap_copy(bitmap1);
	size_t size = (bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] &= bitmap2[i]; }
	return res;
}
size_t* bitmap_and_self(size_t* self, const size_t* const other) {
	if (!self || !other || bitmap_size(self) != bitmap_size(other)) { return NULL; }
	size_t size = (bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] &= other[i]; }
	return self;
}
size_t* bitmap_or(const size_t* const bitmap1, const size_t* const bitmap2) {
	if (!bitmap1 || !bitmap2 || bitmap_size(bitmap1) != bitmap_size(bitmap2)) { return NULL; }
	size_t* res = bitmap_copy(bitmap1);
	size_t size = (bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] |= bitmap2[i]; }
	return res;
}
size_t* bitmap_or_self(size_t* self, const size_t* const other) {
	if (!self || !other || bitmap_size(self) != bitmap_size(other)) { return NULL; }
	size_t size = (bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] |= other[i]; }
	return self;
}
size_t* bitmap_xor(const size_t* const bitmap1, const size_t* const bitmap2) {
	if (!bitmap1 || !bitmap2 || bitmap_size(bitmap1) != bitmap_size(bitmap2)) { return NULL; }
	size_t* res = bitmap_copy(bitmap1);
	size_t size = (bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] ^= bitmap2[i]; }
	return res;
}
size_t* bitmap_xor_self(size_t* self, const size_t* const other) {
	if (!self || !other || bitmap_size(self) != bitmap_size(other)) { return NULL; }
	size_t size = (bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] ^= other[i]; }
	return self;
}

/*
 * Hash Functions
 */

size_t hash_size_t(const void* const val) {
	size_t key = *(size_t*)val;
	key = (~key) + (key << 21);
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8);
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4);
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}
size_t hash_int(const void* const val) {
	int val_int = *(int*)val;
	size_t val_size = ((size_t)val_int << 32) | (size_t)val_int;
	return hash_size_t(&val_size);
}
size_t hash_double(const void* const val) {
	return hash_size_t(val);
}
size_t hash_str(const void* const val) {
    size_t hash = 5381;
	char* str = (char*)val;
	for (size_t i = 0; i < str_length(str); ++i) { 
        hash = ((hash << 5) + hash) + str[i];
	}
    return hash;
}

/*
 * HashSet
 */
typedef struct {
	size_t len;
	size_t cap;
	size_t size;
	
	char is_ptr;

	size_t (*hash_fn)(const void* const);
	int (*eq_fn)(const void* const, const void* const);
	void (*defer_fn)(void*);
	void (*print_fn)(const void* const);
} __HashSetHeader;

#define HASH_SET_HEADER_SIZE (sizeof(__HashSetHeader))
#define HASH_SET_GET_HEADER(set) ((__HashSetHeader*)set - 1)
#define HASH_SET_GET_BITMAP(set) ((size_t*)((char*)set + HASH_SET_GET_HEADER(set)->cap * HASH_SET_GET_HEADER(set)->size) + 1)
#define HASH_SET_BASE_SIZE 32

#define __CONCAT_VALS_BASE__(a, b) a##b
#define __CONCAT_VALS__(a, b) __CONCAT_VALS_BASE__(a, b)
#define __UNIQUE_VAL__(a) __CONCAT_VALS__(a, __LINE__)

#define hashset_size(set) (HASH_SET_GET_HEADER(set)->len)
#define hashset_foreach(val, set) size_t __UNIQUE_VAL__(counter) = 0; \
	for (typeof(*set)* val = set; __UNIQUE_VAL__(counter) < HASH_SET_GET_HEADER(set)->cap; ++__UNIQUE_VAL__(counter), val = (char*)val + HASH_SET_GET_HEADER(set)->size) \
		if (bitmap_get(HASH_SET_GET_BITMAP(set), 2 * __UNIQUE_VAL__(counter)) && !bitmap_get(HASH_SET_GET_BITMAP(set), 2 * __UNIQUE_VAL__(counter) + 1))

void __hashset_add(void** set_ptr, void* val);
struct __HashSetParams {
	size_t __size;
	char is_ptr;

	size_t (*__hash_fn)(const void* const);
	int (*__eq_fn)(const void* const, const void* const);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
};
void* __hashset_new(struct __HashSetParams params) {
	size_t to_alloc = ((HASH_SET_BASE_SIZE * 2 - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t) + HASH_SET_HEADER_SIZE + HASH_SET_BASE_SIZE * params.__size;
	void* ret = malloc(to_alloc);
	memset(ret, 0, to_alloc);

	__HashSetHeader* head = ret;
	void* set = (char*)(head + 1);
	head->cap = HASH_SET_BASE_SIZE;
	head->size = params.__size;
	head->is_ptr = params.is_ptr;
	*(HASH_SET_GET_BITMAP(set) - 1) = 2 * HASH_SET_BASE_SIZE;
	head->hash_fn = params.__hash_fn;
	head->eq_fn = params.__eq_fn;
	head->print_fn = params.print_fn;
	head->defer_fn = params.defer_fn;

	return set;
}
#define __hashset_new_params(...) __hashset_new((struct __HashSetParams){ 0, __VA_ARGS__ })
#define hashset_new(T, hash, eq, ...) (T*)__hashset_new_params(.__size = sizeof(T), .__hash_fn = hash, .__eq_fn = eq, __VA_ARGS__)
void* __hashset_copy(const void* const set) {
	assert(set);
	__HashSetHeader* head = HASH_SET_GET_HEADER(set);

	void* set_res = __hashset_new((struct __HashSetParams){
		.__size = head->size,
		.__hash_fn = head->hash_fn,
		.__eq_fn = head->eq_fn,
		.is_ptr = head->is_ptr,
		.print_fn = head->print_fn,
		.defer_fn = head->defer_fn,
	});
	hashset_foreach(a, set) { __hashset_add(&set_res, (void*)a); }
	return set_res;
}
#define hashset_copy(set) (typeof(*set)*)__hashset_copy(set)
void __hashset_expand(void** set_ptr) {
	void* set = *set_ptr;
	__HashSetHeader* head = HASH_SET_GET_HEADER(set);
	size_t to_alloc = HASH_SET_HEADER_SIZE + (head->cap << 1) * head->size + (((head->cap << 2) - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t);
	void* ret = malloc(to_alloc);
	memset(ret, 0, to_alloc);

	__HashSetHeader* new_head = ret;
	void* new_set = (void*)(new_head + 1);
	memcpy(new_head, head, HASH_SET_HEADER_SIZE);
	new_head->cap <<= 1;

	size_t* bitmap = HASH_SET_GET_BITMAP(set);
	size_t* new_bitmap = HASH_SET_GET_BITMAP(new_set);
	*(new_bitmap - 1) = 2 * new_head->cap;

	for (size_t i = 0; i < head->cap; ++i) {
		if (bitmap_get(bitmap, 2 * i)) {
			void* val = (char*)set + i * head->size;
			size_t pos = new_head->hash_fn(!new_head->is_ptr ? val : *(void**)val) % new_head->cap;
			for (size_t j = 0; j < new_head->cap; ++j) {
				size_t probe = (pos + j * j) % new_head->cap;
				if (!bitmap_get(new_bitmap, 2 * probe)) {
					memcpy((char*)new_set + probe * new_head->size, val, new_head->size);
					bitmap_set(new_bitmap, 2 * probe, 1);
					break;
				}
			}
		}
	}

	free(head);
	*set_ptr = new_set;
}
void __hashset_add(void** set_ptr, void* val) {
	void* set = *set_ptr;
	__HashSetHeader* head = HASH_SET_GET_HEADER(set);
	if (head->len * 1000 > head->cap * 650) {
		__hashset_expand(set_ptr);
		set = *set_ptr;
		head = HASH_SET_GET_HEADER(set);
	}
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");

	size_t* bitmap = HASH_SET_GET_BITMAP(set);
	size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)set + probe * head->size, val, head->size);
			bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)set + probe * head->size, val, head->size);
			bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
			return;
		}
	}
}
#define hashset_add(set, val) do { \
	typeof(*set) v = val; \
	__hashset_add((void**)&set, &v); \
} while(0)
void __hashset_remove(void* set, void* val) {
	assert(set);

	__HashSetHeader* head = HASH_SET_GET_HEADER(set);
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");

	size_t* bitmap = HASH_SET_GET_BITMAP(set);
	size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val);
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (bitmap_get(bitmap, 2 * probe)) {
			
			if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
				bitmap_set(bitmap, 2 * probe + 1, 1);
				if (head->defer_fn) {
					head->defer_fn((char*)set + probe * head->size);
				}
				--head->len;
				return;
			}
		}
	}
}
#define hashset_remove(set, val) do { \
	typeof(*set) v = val; \
	__hashset_remove(set, &v); \
} while(0)
struct __HashSetContainsParams {
	const void* const __set;
	void* __val;
	char defer;
};
int __hashset_contains(struct __HashSetContainsParams params) {
	void* set = (void*)params.__set;
	void* val = params.__val;
	assert(set);

	__HashSetHeader* head = HASH_SET_GET_HEADER(set);
	size_t* bitmap = HASH_SET_GET_BITMAP(set);

	int res = 0;
	for (size_t i = 0; i < head->cap; ++i) {
		if (bitmap_get(bitmap, 2 * i) && !bitmap_get(bitmap, 2 * i + 1)) {
			if (!head->is_ptr ?
					head->eq_fn((char*)set + i * head->size, val) :
					head->eq_fn(*(void**)((char*)set + i * head->size), *(void**)val)) {
				res = 1;
				break;
			} else {
				break;
			}
		}
	}
	if (params.defer && head->defer_fn) {
		head->defer_fn(!head->is_ptr ? val : *(void**)val);
	}
	return res;
}
#define __hashset_contains_params(...) __hashset_contains((struct __HashSetContainsParams){ 0, __VA_ARGS__ })
#define hashset_contains(set, val, ...) ({ \
	typeof(*set) v = val; \
	__hashset_contains_params(.__set = set, .__val = &v, __VA_ARGS__); \
})
void hashset_free(void* set) {
	assert(set);

	__HashSetHeader* head = HASH_SET_GET_HEADER(set);
	if (head->defer_fn) {
		if (!head->is_ptr) {
			hashset_foreach(a, set) { head->defer_fn(a); }
		} else {
			hashset_foreach(a, set) { head->defer_fn(*(void**)a); }
		}
	}
	
	free(head);
}
void hashset_print(const void* set) {
	__HashSetHeader* head = HASH_SET_GET_HEADER(set);
	printf("{ ");
	char started = 0;
	if (!head->is_ptr) {
		hashset_foreach(a, set) {
			if (started) { printf(", "); } else { started = 1; }
			head->print_fn(a);
		}
	} else {
		hashset_foreach(a, set) {
			if (started) { printf(", "); } else { started = 1; }
			head->print_fn(*(void**)a);
		}
	}
	printf(" }");
}

// TODO: add self
void* __hashset_union(const void* const set1, const void* const set2) {
	assert(set1 && set2);
	__HashSetHeader* head1 = HASH_SET_GET_HEADER(set1);
	__HashSetHeader* head2 = HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __hashset_copy(set1);
	hashset_foreach(a, set2) { __hashset_add(&set_res, (void*)a); }

	return set_res;
}
#define hashset_union(set1, set2) (typeof(*set1)*)__hashset_union(set1, set2)
void* hashset_union_self(void* self, const void* const other);
void* __hashset_intersec(const void* const set1, const void* const set2) {
	assert(set1 && set2);
	__HashSetHeader* head1 = HASH_SET_GET_HEADER(set1);
	__HashSetHeader* head2 = HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __hashset_copy(set1);
	hashset_foreach(a, set1) {
		if (!__hashset_contains_params(.__set = set2, .__val = (void*)a, .defer = 0)) {
			__hashset_remove(set_res, (void*)a);
		}
	}

	return set_res;
}
#define hashset_intersec(set1, set2) (typeof(*set1)*)__hashset_intersec(set1, set2)
void* hashset_intersec_self(void* self, const void* const other);
void* __hashset_diff(const void* const set1, const void* const set2) {
	assert(set1 && set2);
	__HashSetHeader* head1 = HASH_SET_GET_HEADER(set1);
	__HashSetHeader* head2 = HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __hashset_copy(set1);
	hashset_foreach(a, set2) { __hashset_remove(set_res, (void*)a); }

	return set_res;
}
#define hashset_diff(set1, set2) (typeof(*set1)*)__hashset_diff(set1, set2)
void* hashset_diff_self(void* self, const void* const other);

/*
 * HashMap
 */

typedef struct {
	size_t len;
	size_t cap;
	size_t size_key;
	size_t size_value;
	size_t size;
	
	char is_key_ptr;
	char is_value_ptr;

	size_t (*hash_fn)(const void* const);
	int (*eq_fn)(const void* const, const void* const);
	void (*defer_key_fn)(void*);
	void (*defer_value_fn)(void*);
	void (*print_key_fn)(const void* const);
	void (*print_value_fn)(const void* const);
} __HashMapHeader;

#define HASHMAP_BASE_SIZE 32
#define HASHMAP_HEADER_SIZE (sizeof(__HashMapHeader))
#define HASHMAP_GET_HEADER(map) ((__HashMapHeader*)(map) - 1)
#define HASHMAP_GET_BITMAP(map) ((size_t*)((char*)(map) + HASHMAP_GET_HEADER(map)->size * HASHMAP_GET_HEADER(map)->cap) + 1)

#define hashmap_size(map) (HASHMAP_GET_HEADER(map)->len)

#define hashmap_foreach(val, map) size_t __UNIQUE_VAL__(i) = 0; \
	for (typeof(map->value)* val = (void*)((char*)map + HASHMAP_GET_HEADER(map)->size_key); \
		__UNIQUE_VAL__(i) < HASHMAP_GET_HEADER(map)->cap; \
		++__UNIQUE_VAL__(i), \
		val = (void*)((char*)val + HASHMAP_GET_HEADER(map)->size)) \
		if (bitmap_get(HASHMAP_GET_BITMAP(map), 2 * __UNIQUE_VAL__(i)) && !bitmap_get(HASHMAP_GET_BITMAP(map), 2 * __UNIQUE_VAL__(i) + 1))

struct __HashMapParams {
	size_t __size_key;
	size_t __size_value;
	
	char is_key_ptr;
	char is_value_ptr;

	size_t (*__hash_fn)(const void* const);
	int (*__eq_fn)(const void* const, const void* const);
	void (*defer_key_fn)(void*);
	void (*defer_value_fn)(void*);
	void (*print_key_fn)(const void* const);
	void (*print_value_fn)(const void* const);
};
void* __hashmap_new(struct __HashMapParams params) {
	size_t alloc_size = HASHMAP_HEADER_SIZE +
			HASHMAP_BASE_SIZE * (params.__size_key + params.__size_value) +
			((2 * HASHMAP_BASE_SIZE - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t);
	__HashMapHeader* head = malloc(alloc_size);
	memset(head, 0, alloc_size);

	head->len = 0;
	head->cap = HASHMAP_BASE_SIZE;
	head->size_key = params.__size_key;
	head->size_value = params.__size_value;
	head->size = params.__size_value + params.__size_key;
	
	head->is_key_ptr = params.is_key_ptr;
	head->is_value_ptr = params.is_value_ptr;

	head->hash_fn = params.__hash_fn;
	head->eq_fn = params.__eq_fn;
	head->defer_key_fn = params.defer_key_fn;
	head->defer_value_fn = params.defer_value_fn;
	head->print_key_fn = params.print_key_fn;
	head->print_value_fn = params.print_value_fn;
	
	void* map = head + 1;
	bitmap_size(HASHMAP_GET_BITMAP(map)) = 2 * HASHMAP_BASE_SIZE;
	return map;
}
#define __hashmap_new_params(...) __hashmap_new((struct __HashMapParams){ 0, __VA_ARGS__ })
#define hashmap_new(T, hash, eq, ...) (T*)__hashmap_new_params(.__size_key = sizeof((T){0}.key), .__size_value = sizeof((T){0}.value), .__hash_fn = hash, .__eq_fn = eq, __VA_ARGS__)
void __hashmap_expand(void** map_ptr) {
	void* map = *map_ptr;
	__HashMapHeader* head = HASHMAP_GET_HEADER(map);
	size_t* bitmap = HASHMAP_GET_BITMAP(map);

	size_t alloc_size = HASHMAP_HEADER_SIZE + 
			(head->cap << 1) * head->size +
			(((head->cap << 2) - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t);
	__HashMapHeader* new_head = malloc(alloc_size);

	memset(new_head, 0, alloc_size);
	memcpy(new_head, head, HASHMAP_HEADER_SIZE);
	new_head->len = 0;
	new_head->cap <<= 1;
	void* new_map = new_head + 1;
	size_t* new_bitmap = HASHMAP_GET_BITMAP(new_map);
	bitmap_size(new_bitmap) = new_head->cap << 1;
	
	for (size_t i = 0; i < head->cap; ++i) {
		if (!bitmap_get(bitmap, 2 * i) || bitmap_get(bitmap, 2 * i + 1)) { continue; }

		void* key = (char*)map + i * head->size;
		size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % new_head->cap;
		for (size_t j = 0; j < new_head->cap; ++j) {
			size_t probe = (pos + j * j) % new_head->cap;
			if (!bitmap_get(new_bitmap, 2 * probe)) {
				memcpy((char*)new_map + probe * head->size, key, head->size);
				bitmap_set(new_bitmap, 2 * probe, 1);
				++head->len;
				break;
			} else if (bitmap_get(new_bitmap, 2 * probe + 1)) {
				memcpy((char*)new_map + probe * head->size, key, head->size);
				bitmap_set(new_bitmap, 2 * probe + 1, 0);
				++new_head->len;
				break;
			}
		}
	}

	free(head);
	*map_ptr = new_map;
}
void __hashmap_add(void** map_ptr, void* key) {
	void* map = *map_ptr;
	__HashMapHeader* head = HASHMAP_GET_HEADER(map);
	if (head->len * 1000 >= head->cap * 650) {
		__hashmap_expand(map_ptr);
		map = *map_ptr;
		head = HASHMAP_GET_HEADER(map);
	}

	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = HASHMAP_GET_BITMAP(map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_key_ptr ? head->eq_fn((char*)map + probe * head->size, key) : head->eq_fn(*(void**)((char*)map + probe * head->size), *(void**)key)) {
			return;
		}

	}
}
#define hashmap_add(map, k) do { \
	typeof(map->key) key = k; \
	__hashmap_add((void**)&(map), &key); \
} while(0)
void __hashmap_add_v(void** map_ptr, void* key, void* val) {
	void* map = *map_ptr;
	__HashMapHeader* head = HASHMAP_GET_HEADER(map);
	if (head->len * 1000 >= head->cap * 650) {
		__hashmap_expand(map_ptr);
		map = *map_ptr;
		head = HASHMAP_GET_HEADER(map);
	}

	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = HASHMAP_GET_BITMAP(map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_key_ptr ? head->eq_fn((char*)map + probe * head->size, key) : head->eq_fn(*(void**)((char*)map + probe * head->size), *(void**)key)) {
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			return;
		}

	}
}
#define hashmap_add_v(map, k, v) do { \
	typeof(map->key) key = k; \
	typeof(map->value) val = v; \
	__hashmap_add_v((void**)&(map), &key, &val); \
} while(0)
struct __HashMapFuncParams {
	void* __map;
	void* __key;
	char defer;
};
void* __hashmap_get(struct __HashMapFuncParams params) {
	assert(params.__map);
	__HashMapHeader* head = HASHMAP_GET_HEADER(params.__map);
	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = HASHMAP_GET_BITMAP(params.__map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key) % head->cap;

	void* res = NULL;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (bitmap_get(bitmap, 2 * probe) && !bitmap_get(bitmap, 2 * probe + 1)) {
			if (!head->is_key_ptr ?
				 head->eq_fn((char*)params.__map + probe * head->size, params.__key) :
				 head->eq_fn(*(void**)((char*)params.__map + probe * head->size), *(void**)params.__key)) {
				res = (char*)params.__map + probe * head->size + head->size_key;
				break;
			} else {
				break;
			}
		}
	}
	if (params.defer && head->defer_key_fn) {
		head->defer_key_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key);
	}
	return res;
}
#define __hashmap_get_params(...) __hashmap_get((struct __HashMapFuncParams){ 0, __VA_ARGS__ })
#define hashmap_get(map, k, ...) ({ \
	typeof(map->key) key = k; \
	(typeof(map->value)*) __hashmap_get_params( .__map = map, .__key = &key, __VA_ARGS__ ); \
})
void __hashmap_remove(struct __HashMapFuncParams params) {
	assert(params.__map);
	__HashMapHeader* head = HASHMAP_GET_HEADER(params.__map);
	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = HASHMAP_GET_BITMAP(params.__map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key) % head->cap;

	int res = -1;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (bitmap_get(bitmap, 2 * probe) && !bitmap_get(bitmap, 2 * probe + 1)) {
			if (!head->is_key_ptr ?
				 head->eq_fn((char*)params.__map + probe * head->size, params.__key) :
				 head->eq_fn(*(void**)((char*)params.__map + probe * head->size), *(void**)params.__key)) {
				res = (int)probe;
				break;
			} else {
				break;
			}
		}
	}
	if (params.defer && head->defer_key_fn) {
		head->defer_key_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key);
	}
	if (res >= 0) {
		--head->len;
		bitmap_set(bitmap, 2 * res + 1, 1);
		void* key = (char*)params.__map + res * head->size;
		if (head->defer_key_fn) {
			head->defer_key_fn(!head->is_key_ptr ? key : *(void**)key);
		}
		key = (char*)key + head->size_key;
		if (head->defer_value_fn) {
			if (!head->is_value_ptr) {
				head->defer_value_fn(key);
			} else if (*(void**)key) {
				head->defer_value_fn(*(void**)key);
			}
		}
	}
}
#define __hashmap_remove_params(...) __hashmap_remove((struct __HashMapFuncParams){ 0, __VA_ARGS__ })
#define hashmap_remove(map, k, ...) do { \
	typeof(map->key) key = k; \
	__hashmap_remove_params( .__map = map, .__key = &key, __VA_ARGS__ ); \
} while(0)
void hashmap_print(const void* map) {
	__HashMapHeader* head = HASHMAP_GET_HEADER(map);
	assert(head->print_key_fn && head->print_value_fn);
	size_t* bitmap = HASHMAP_GET_BITMAP(map);

	printf("{ ");
	char started = 0;
	for (size_t i = 0; i < head->cap; ++i) {
		if (!bitmap_get(bitmap, 2 * i) || bitmap_get(bitmap, 2 * i + 1)) { continue; }

		if (started) { printf(", "); } else { started = 1; }
		void* key = (char*)map + i * head->size;
		void* val = (char*)map + i * head->size + head->size_key;
		head->print_key_fn(!head->is_key_ptr ? key : *(void**)key);
		printf(" : ");
		head->print_value_fn(!head->is_value_ptr ? val : *(void**)val);
	}
	printf(" }");
}
void hashmap_free(void* map) {
	assert(map);
	
	__HashMapHeader* head = HASHMAP_GET_HEADER(map);
	size_t* bitmap = HASHMAP_GET_BITMAP(map);
	if (head->defer_key_fn) {
		if (!head->is_key_ptr) {
			for (size_t i = 0; i < head->cap; ++i) {
				if (bitmap_get(bitmap, 2 * i) && !bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_key_fn((char*)map + i * head->size);
				}
			}
		} else {
			for (size_t i = 0; i < head->cap; ++i) {
				if (bitmap_get(bitmap, 2 * i) && !bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_key_fn(*(void**)((char*)map + i * head->size));
				}
			}
		}
	}

	if (head->defer_value_fn) {
		if (!head->is_value_ptr) {
			for (size_t i = 0; i < head->cap; ++i) {
				if (bitmap_get(bitmap, 2 * i) && !bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_key_fn((char*)map + i * head->size + head->size_key);
				}
			}
		} else {
			for (size_t i = 0; i < head->cap; ++i) {
				if (bitmap_get(bitmap, 2 * i) && !bitmap_get(bitmap, 2 * i + 1)) {
					void* val = *(void**)((char*)map + i * head->size + head->size_key);
					if (val) {
						head->defer_key_fn(val);
					}
				}
			}
		}
	}
	free(head);
}

// additional functions for examples
int int_compare(const void* a, const void* b) {
	int aa = *(int*)a;
	int bb = *(int*)b;
	return aa < bb ? -1 : (aa > bb ? 1 : 0);
}
void int_print(const void* n) { printf("%d", *(int*)n); }
void int_pow(void* dst, const void* src) { *(int*)dst = *(int*)src * *(int*)src; }
int int_filter(const void* n) { return *(int*)n % 2; }

int int_eq(const void* const a, const void* const b) { return *(int*)a == *(int*)b; }

void set_has_string(char** set, const char* str) {
	if (hashset_contains(set, str_from_lit(str), .defer = 1)) {
		printf("Set does contain string [\"%s\"]\n", str);
	} else {
		printf("Set doesn\'t contain string [\"%s\"]\n", str);
	}
}

typedef struct { char* key; int value; } KV;

// examples
int main() {
	srand(time(NULL));
	// int array example
	printf("Array examples:\n"); {
		int* array = array_new(int, .cmp_fn = int_compare, .print_fn = int_print);
		for (size_t i = 0; i < 25; ++i) { array_append(array, rand() % 50); }
		array_print(array);
		putchar('\n');

		int* new_arr = array_map(array, int_pow);
		array_filter_self(new_arr, int_filter);
		array_print(new_arr);
		putchar('\n');

		array_sort(array);
		array_print(array);
		putchar('\n');

		array_free(array);
		array_free(new_arr);

		// 2d int array with defering and printing example
		int** arr2d = array_new(int*, .is_ptr = 1, .defer_fn = array_free, .print_fn = array_print);
		for (size_t i = 0; i < 5; ++i) {
			int* arr1d = array_new(int, .print_fn = int_print);
			for (size_t j = 0; j < 5; ++j) {
				array_append(arr1d, rand() % 20);
			}
			array_append(arr2d, arr1d);
		}
		array_print(arr2d);
		putchar('\n');
		array_free(arr2d);
	}

	// string example
	printf("\nString examples:\n"); {
		char* str = str_from_lit("hello");
		printf("A string: \""STR_FMT"\"\n", STR_UNPACK(str));
		str_free(str);

		char** str_arr = array_new(char*, .is_ptr = 1, .defer_fn = str_free, .cmp_fn = str_cmp, .print_fn = str_print);
		char* strs[5] = { "hello", "world", "how", "are", "you" };
		for (size_t i = 0; i < 5; ++i) {
			char* new_str = str_from_lit(strs[i]);
			array_append(str_arr, new_str);
		}
		array_print(str_arr);
		putchar('\n');

		array_sort(str_arr);
		array_print(str_arr);
		putchar('\n');
		array_free(str_arr);
	}

	// bitmap example
	printf("\nBitmap examples:\n"); {
		size_t* bits1 = bitmap_new(20);
		size_t* bits2 = bitmap_new(bitmap_size(bits1));

		for (size_t i = 0; i < bitmap_size(bits1); ++i) {
			int val = rand() % 2;
			bitmap_set(bits1, i, val);
			val = rand() % 3;
			bitmap_set(bits2, i, val);
		}

		printf("  ");
		bitmap_print(bits1);
		printf("\n^ ");
		bitmap_print(bits2);
		bitmap_xor_self(bits1, bits2);
		printf("\n= ");
		bitmap_print(bits1);
		putchar('\n');

		bitmap_free(bits1);
		bitmap_free(bits2);
	}

	// hashset example
	printf("\nHashSet examples:\n"); {
		int* int_set = hashset_new(int, hash_int, int_eq, .print_fn = int_print);
		for (size_t i = 0; i < 75; ++i) {
			hashset_add(int_set, rand() % 50);
		}
		printf("%zu: ", hashset_size(int_set));
		hashset_print(int_set);
		putchar('\n');
		for (size_t i = 0; i < 50; i += 5) {
			hashset_remove(int_set, i);
		}
		printf("%zu: ", hashset_size(int_set));
		hashset_print(int_set);
		putchar('\n');
		hashset_free(int_set);

		// hashset functions testing
		int* set1 = hashset_new(int, hash_int, int_eq, .print_fn = int_print);
		hashset_add(set1, 5);
		hashset_add(set1, 7);
		int* set2 = hashset_copy(set1);
		hashset_add(set2, 8);
		hashset_add(set2, 9);
		hashset_add(set2, 1);
		hashset_add(set2, 11);

		hashset_add(set1, 2);
		hashset_add(set1, 13);

		printf("A =\t");
		hashset_print(set1);
		printf("\nB =\t");
		hashset_print(set2);
		printf("\nA u B =\t");
		hashset_print(hashset_union(set1, set2));
		printf("\nA n B =\t");
		hashset_print(hashset_intersec(set1, set2));
		printf("\nA - B =\t");
		hashset_print(hashset_diff(set1, set2));
		printf("\nB - A =\t");
		hashset_print(hashset_diff(set2, set1));
		putchar('\n');

		hashset_free(set1);
		hashset_free(set2);

		char* strs[7] = { "hello", "world", "how", "are", "you", "wowy", "yes" };
		char** str_set = hashset_new(char*, hash_str, str_eq, .print_fn = str_print, .defer_fn = str_free, .is_ptr = 1);
		for (size_t i = 0; i < 10; ++i) {
			hashset_add(str_set, str_from_lit(strs[(i * 2) % 7]));
		}
		hashset_print(str_set);
		putchar('\n');
		set_has_string(str_set, "you");
		set_has_string(str_set, "no");
		hashset_free(str_set);
	}
	
	// hashmap examples
	printf("\nHashMap examples:\n"); {
		KV* str_to_int = hashmap_new(KV, hash_str, str_eq, .is_key_ptr = 1, .defer_key_fn = str_free, .print_key_fn = str_print, .print_value_fn = int_print);

		char* strs[10] = { "hello", "world", "how", "are", "you", "wowy", "yes", "a", "abc", "xyz" };
		for (size_t i = 0; i < 10; ++i) {
			hashmap_add_v(str_to_int, str_from_lit(strs[i]), rand() % 200 - 100);
		}

		hashmap_print(str_to_int);
		putchar('\n');

		int* abc = hashmap_get(str_to_int, str_from_lit("abc"), .defer = 1);
		if (abc) {
			printf("\"abc\" = %d\n", *abc);
			*abc = 69;
		}
		int* hello = hashmap_get(str_to_int, str_from_lit("hello"), .defer = 1);
		if (hello) {
			printf("\"hello\" = %d\n", *hello);
			*hello = 420;
		}

		printf("%zu: ", hashmap_size(str_to_int));
		hashmap_print(str_to_int);
		putchar('\n');

		for (size_t i = 0; i < 10; ++i) {
			int r = rand() % 10;
			char* str = str_from_lit(strs[r]);
			printf("Removed: [\""STR_FMT"\"]\n",  STR_UNPACK(str));
			hashmap_remove(str_to_int, str, .defer = 1);
		}

		printf("%zu: ", hashmap_size(str_to_int));
		hashmap_print(str_to_int);
		putchar('\n');

		hashmap_free(str_to_int);
	}
	return 0;
}
