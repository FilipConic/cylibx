#ifndef __CYLIBX_H__
#define __CYLIBX_H__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#define __CYX_CLOSE_FOLD 1

/*
 * Temporary Buffer
 */

#if __CYX_CLOSE_FOLD
#define __CYX_TEMP_BUFFER_SIZE 8192
static struct {
	char buffer[__CYX_TEMP_BUFFER_SIZE];
	char* curr;
	char* marker;
} __cyx_temp = { 0 };
void* __cyx_temp_buffer_alloc(size_t n_bytes);
#define __cyx_temp_buffer_reset() do { __cyx_temp.curr = __cyx_temp.buffer; } while(0)
#define __cyx_temp_buffer_marker_set() do { __cyx_temp.marker = __cyx_temp.curr; } while (0)
#define __cyx_temp_buffer_marker_reset() do { __cyx_temp.curr = __cyx_temp.marker; } while(0)

#define __CYX_DELETED_PTR_COUNT 64
#define __CYX_DELETED_PTR_SIZE (64 * __CYX_DELETED_PTR_COUNT)
struct __DeletedFN {
	void(*defer_fn)(void*);
	void* byte_pos;
	char is_ptr;
};
static struct {
	struct __DeletedFN fns[__CYX_DELETED_PTR_COUNT];
	char buffer[__CYX_DELETED_PTR_SIZE];
	size_t fn_count;
	size_t buffer_count;
} __cyx_temp_deleted = { 0 };

void __cyx_temp_reset_deleted();
void* __cyx_temp_alloc_deleted(size_t bytes, void* val, char is_ptr, void(*fn)(void*));

#ifdef CYLIBX_IMPLEMENTATION

void* __cyx_temp_buffer_alloc(size_t n_bytes) {
	if (!__cyx_temp.curr) { __cyx_temp.curr = __cyx_temp.buffer; }
	if ((size_t)__cyx_temp.curr + n_bytes > (size_t)__cyx_temp.buffer + __CYX_TEMP_BUFFER_SIZE) { return NULL; }
	void* ret = __cyx_temp.curr;
	__cyx_temp.curr += n_bytes;
	return ret;
}
void __cyx_temp_reset_deleted() {
	if (!__cyx_temp_deleted.buffer_count && !__cyx_temp_deleted.fn_count) { return; }

	for (size_t i = 0; i < __cyx_temp_deleted.fn_count; ++i) {
		struct __DeletedFN* curr = &__cyx_temp_deleted.fns[i];
		curr->defer_fn(!curr->is_ptr ? curr->byte_pos : *(void**)(curr->byte_pos));
	}

	__cyx_temp_deleted.fn_count = 0;
	__cyx_temp_deleted.buffer_count = 0;
}
void* __cyx_temp_alloc_deleted(size_t bytes, void* val, char is_ptr, void(*fn)(void*)) {
	if (__cyx_temp_deleted.buffer_count + bytes > __CYX_DELETED_PTR_SIZE || __cyx_temp_deleted.fn_count == __CYX_DELETED_PTR_COUNT) { __cyx_temp_reset_deleted(); }
	void* ret = (char*)__cyx_temp_deleted.buffer;

	memcpy(ret, val, bytes);
	__cyx_temp_deleted.buffer_count += bytes;
	__cyx_temp_deleted.fns[__cyx_temp_deleted.fn_count++] = (struct __DeletedFN){ .defer_fn = fn, .byte_pos = ret, .is_ptr = is_ptr, };

	return ret;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Util macros
 */

#if __CYX_CLOSE_FOLD

#define __CYX_DATA_GET_AT(header, data_ptr, pos) ((char*)(data_ptr) + (pos) * (header)->size)
#define __CYX_PTR_CMP(header, l, r) (!(header)->is_ptr ? (header)->cmp_fn(l, r) : (header)->cmp_fn(*(void**)(l), *(void**)(r)))
#define __CYX_PTR_CMP_POS(header, data_ptr, l_pos, r_pos) (!(header)->is_ptr ? \
	(header)->cmp_fn(__CYX_DATA_GET_AT(header, data_ptr, l_pos), __CYX_DATA_GET_AT(header, data_ptr, r_pos)) : \
	(header)->cmp_fn(*(void**)(__CYX_DATA_GET_AT(header, data_ptr, l_pos)), *(void**)(__CYX_DATA_GET_AT(header, data_ptr, r_pos))) \
)
#define __CYX_SWAP(a, b, size) do { \
	__cyx_temp_buffer_marker_set(); \
		void* temp = __cyx_temp_buffer_alloc(size); \
		assert(temp && "ERROR: Can not allocate to the temporary buffer anymore!"); \
		memcpy(temp, a, size); \
		memcpy(a, b, size); \
		memcpy(b, temp, size); \
	__cyx_temp_buffer_marker_reset(); \
} while(0);
#define __CYX_CONCAT_VALS_BASE__(a, b) a##b
#define __CYX_CONCAT_VALS__(a, b) __CYX_CONCAT_VALS_BASE__(a, b)
#define __CYX_UNIQUE_VAL__(a) __CYX_CONCAT_VALS__(a, __LINE__)

#endif // __CYX_CLOSE_FOLD

/*
 * String
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
} __CyxStringHeader;

#ifndef CYX_STRING_BASE_SIZE
#define CYX_STRING_BASE_SIZE 16
#endif // CYX_STRING_BASE_SIZE

#define __CYX_STRING_HEADER_SIZE (sizeof(__CyxStringHeader))
#define __CYX_STRING_GET_HEADER(str) ((__CyxStringHeader*)str - 1)

char* cyx_str_new();
char* cyx_str_from_lit_n(const char* c_str, size_t n);
void cyx_str_free(void* str);
void cyx_str_print(const void* str);
int cyx_str_eq(const void* val1, const void* val2);
int cyx_str_cmp(const void* val1, const void* val2);

#define cyx_str_length(str) (__CYX_STRING_GET_HEADER(str)->len)
#define CYX_STR_FMT "%.*s"
#define CYX_STR_UNPACK(str) (int)cyx_str_length(str), str

#define cyx_str_from_lit(c_str) cyx_str_from_lit_n(c_str, strlen(c_str))
#define cyx_str_copy(str) cyx_str_from_lit_n(str, __CYX_STRING_GET_HEADER(str)->len)

#ifdef CYLIBX_STRIP_PREFIX

#define STR_FMT CYX_STR_FMT
#define STR_UNPACK(str) CYX_STR_UNPACK(str)

#define str_length(str) cyx_str_length(str)
#define str_from_lit(c_str) cyx_str_from_lit(c_str)
#define str_copy(str) cyx_str_copy(str)

#define str_new cyx_str_new
#define str_from_lit_n cyx_str_from_lit_n
#define str_free cyx_str_free
#define str_print cyx_str_print
#define str_eq cyx_str_eq
#define str_cmp cyx_str_cmp

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

char* cyx_str_new() {
	__CyxStringHeader* head = malloc(__CYX_STRING_HEADER_SIZE + CYX_STRING_BASE_SIZE * sizeof(char));
	assert(head);
	memset(head, 0, __CYX_STRING_HEADER_SIZE + CYX_STRING_BASE_SIZE * sizeof(char));
	head->cap = CYX_STRING_BASE_SIZE;
	char* str = (char*)(head + 1);
	return str;
}
char* cyx_str_from_lit_n(const char* c_str, size_t n) {
	__CyxStringHeader* head = malloc(__CYX_STRING_HEADER_SIZE + n * sizeof(char));
	memcpy(head + 1, c_str, n * sizeof(char));
	head->cap = n;
	head->len = n;
	return (char*)(head + 1);
}
void cyx_str_free(void* str) {
	assert(str);
	free(__CYX_STRING_GET_HEADER((void*)str));
}
void cyx_str_print(const void* str) {
	printf("\""CYX_STR_FMT"\"", CYX_STR_UNPACK((char*)str));
}
int cyx_str_eq(const void* val1, const void* val2) {
	char* str1 = (char*)val1;
	char* str2 = (char*)val2;
	if (cyx_str_length(str1) != cyx_str_length(str2)) { return 0; }
	for (size_t i = 0; i < cyx_str_length(str1); ++i) {
		if (str1[i] != str2[i]) { return 0; }
	}
	return 1;
}
int cyx_str_cmp(const void* val1, const void* val2) {
	char* str1 = (char*)val1;
	char* str2 = (char*)val2;
	int ret = strncmp(str1, str2, cyx_str_length(str1) < cyx_str_length(str2) ? cyx_str_length(str1) : cyx_str_length(str2)); 
	if (ret < 0) return -1;
	else if (ret > 0) return 1;
	else return 0;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Array
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t size;
	size_t len;
	size_t cap;

	void (*defer_fn)(void*);
	int (*cmp_fn)(const void*, const void*);
	void (*print_fn)(const void*);

	char is_ptr;
} __CyxArrayHeader;

struct __CyxArrayParams {
	size_t __size;
	size_t reserve;
	char is_ptr;

	void (*defer_fn)(void*);
	int (*cmp_fn)(const void*, const void*);
	void (*print_fn)(const void*);
};

#ifndef CYX_ARRAY_BASE_SIZE
#define CYX_ARRAY_BASE_SIZE 16
#endif // CYX_ARRAY_BASE_SIZE

#define __CYX_ARRAY_HEADER_SIZE (sizeof(__CyxArrayHeader))
#define __CYX_ARRAY_GET_HEADER(arr) ((__CyxArrayHeader*)(arr) - 1)

void* __cyx_array_new(struct __CyxArrayParams params);
void* __cyx_array_copy(void* arr);
void __cyx_array_expand(void** arr_ptr, size_t n);
void __cyx_array_append(void** arr, void* val);
void* __cyx_array_remove(void* arr, int pos);
void* __array_at(void* arr, int pos);
void cyx_array_free(void* arr);
void __cyx_array_append_mult_n(void** arr_ptr, size_t n, const void* mult);
void cyx_array_print(const void* arr);

void __cyx_array_sort(void* arr, int start, int end);
void* __cyx_array_map(const void* const arr, void (*fn)(void*, const void*));
void* cyx_array_map_self(void* arr, void (*fn)(void*, const void*));
void* __cyx_array_filter(const void* const arr, int (*fn)(const void*));
void* cyx_array_filter_self(void* arr, int (*fn)(const void*));
int cyx_array_find(void* arr, void* val);
int cyx_array_find_by(void* arr, int (*fn)(const void*));

#define cyx_array_length(arr) (__CYX_ARRAY_GET_HEADER(arr)->len)
#define cyx_array_foreach(val, arr) for ( struct { typeof(*arr)* value; size_t idx; } val = { .value = arr, .idx = 0 }; val.idx < array_length(arr); ++val.idx, val.value = (typeof(*arr)*)((char*)val.value + __CYX_ARRAY_GET_HEADER(arr)->size) )
#define cyx_array_drain(val, arr) for (typeof(*arr)* val = NULL; (val = array_pop(arr));)

#define __cyx_array_new_params(...) __cyx_array_new((struct __CyxArrayParams){ 0, __VA_ARGS__ })
#define cyx_array_new(T, ...) (T*)__cyx_array_new_params(.__size = sizeof(T), __VA_ARGS__)
#define cyx_array_copy(arr) (typeof(*arr)*)__cyx_array_copy(arr)
#define cyx_array_append(arr, val) do { \
	typeof(*arr) v = (val); \
	__cyx_array_append((void**)&arr, &v); \
} while(0)
#define cyx_array_append_mult_n(arr, n, mult) __cyx_array_append_mult_n((void**)&(arr), n, mult)
#define cyx_array_append_mult(arr, ...) do { \
	typeof(*arr) mult[] = { __VA_ARGS__ }; \
	__cyx_array_append_mult_n((void**)&(arr), sizeof(mult)/sizeof(*(mult)), mult); \
} while(0)
#define cyx_array_remove(arr, pos) (typeof(*arr)*)__cyx_array_remove(arr, pos)
#define cyx_array_pop(arr) (typeof(*arr)*)__cyx_array_remove(arr, -1)
#define cyx_array_at(arr, pos) (typeof(*arr)*)__cyx_array_at(arr, pos)
#define cyx_array_set_cmp(arr, cmp) do { __CYX_ARRAY_GET_HEADER(arr)->cmp_fn = cmp; } while(0)
#define cyx_array_sort(arr) do { \
	assert(arr); \
	assert(__CYX_ARRAY_GET_HEADER(arr)->cmp_fn && "ERROR: Trying to sort without a compare function provided!"); \
	__cyx_array_sort(arr, 0, __CYX_ARRAY_GET_HEADER(arr)->len - 1); \
} while(0)
#define cyx_array_map(arr, fn) (typeof(*arr)*)__cyx_array_map(arr, fn)
#define cyx_array_filter(arr, fn) (typeof(*arr)*)__cyx_array_filter(arr, fn)

#ifdef CYLIBX_STRIP_PREFIX

#define array_length(arr) cyx_array_length(arr)
#define array_foreach(val, arr) cyx_array_foreach(val, arr)
#define array_drain(val, arr) cyx_array_drain(val, arr)

#define array_new(T, ...) cyx_array_new(T, __VA_ARGS__) 
#define array_copy(arr) cyx_array_copy(arr)
#define array_append(arr, val) cyx_array_append(arr, val)
#define array_append_mult_n(arr, n, mult) cyx_array_append_mult_n(arr, n, mult)
#define array_append_mult(arr, ...) cyx_array_append_mult(arr, __VA_ARGS__)
#define array_remove(arr, pos) cyx_array_remove(arr, pos)
#define array_pop(arr) cyx_array_pop(arr)
#define array_at(arr, pos) cyx_array_at(arr, pos)
#define array_set_cmp(arr, cmp) cyx_array_set_cmp(arr, cmp)
#define array_sort(arr) cyx_array_sort(arr)
#define array_map(arr, fn) cyx_array_map(arr, fn)
#define array_filter(arr, fn) cyx_array_filter(arr, fn)

#define array_free cyx_array_free
#define array_print cyx_array_print
#define array_map_self cyx_array_map_self
#define array_filter_self cyx_array_filter_self
#define array_find cyx_array_find
#define array_find_by cyx_array_find_by

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

void* __cyx_array_new(struct __CyxArrayParams params) {
	__CyxArrayHeader* arr;
	if (!params.reserve) {
		arr = malloc(__CYX_ARRAY_HEADER_SIZE + CYX_ARRAY_BASE_SIZE * params.__size);
		if (!arr) { return NULL; }
		arr->cap = CYX_ARRAY_BASE_SIZE;
	} else {
		arr = malloc(__CYX_ARRAY_HEADER_SIZE + params.reserve * params.__size);
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
void* __cyx_array_copy(void* arr) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	void* res = malloc(__CYX_ARRAY_HEADER_SIZE + head->cap * head->size);
	if (!res) { return NULL; }
	__CyxArrayHeader* res_head = res;
	memcpy(res_head, head, __CYX_ARRAY_HEADER_SIZE + head->len * head->size);

	return (void*)(res_head + 1);
}
void __cyx_array_expand(void** arr_ptr, size_t n) {
	assert(*arr_ptr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(*arr_ptr);
	size_t new_cap = head->cap;
	while (n + head->len > (new_cap <<= 1));
	__CyxArrayHeader* new_head = malloc(__CYX_ARRAY_HEADER_SIZE + new_cap * head->size);
	memcpy(new_head, head, __CYX_ARRAY_HEADER_SIZE + head->cap * head->size);
	new_head->cap = new_cap;

	free(head);
	*arr_ptr = new_head + 1;
}
void __cyx_array_append(void** arr, void* val) {
	assert(*arr);
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(*arr);
	if (head->len == head->cap) {
		__cyx_array_expand(arr, 1);
		head = __CYX_ARRAY_GET_HEADER(*arr);
	}
	memcpy(__CYX_DATA_GET_AT(head, *arr, head->len++), val, head->size);
}
void* __cyx_array_remove(void* arr, int pos) {
	assert(arr);
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (pos < 0) { pos += head->len; }
	assert(pos < (int)head->len);
	--head->len;

	void* ret;
	if (head->defer_fn) {
		memcpy(__CYX_DATA_GET_AT(head, arr, pos), __CYX_DATA_GET_AT(head, arr, head->len), head->size);
		ret = __cyx_temp_alloc_deleted(head->size, __CYX_DATA_GET_AT(head, arr, head->len), head->is_ptr, head->defer_fn);
	} else {
		__CYX_SWAP(__CYX_DATA_GET_AT(head, arr, pos), __CYX_DATA_GET_AT(head, arr, head->len), head->size);
		ret = __CYX_DATA_GET_AT(head, arr, head->len);
	}
	return ret;
}
void* __array_at(void* arr, int pos) {
	if (!arr) { return NULL; }

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (pos < 0) { pos += head->len; }
	if (pos >= (int)head->len) { return NULL; }

	return (char*)arr + pos * head->size;
}
void cyx_array_free(void* arr) {
	assert(arr);
	__cyx_temp_reset_deleted();

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
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
void __cyx_array_append_mult_n(void** arr_ptr, size_t n, const void* mult) {
	void* arr = *arr_ptr;
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (head->len + n >= head->cap) {
		__cyx_array_expand(arr_ptr, n);
		arr = *arr_ptr;
		head = __CYX_ARRAY_GET_HEADER(arr);
	}

	memcpy(__CYX_DATA_GET_AT(head, arr, head->len), mult, n * head->size);
	head->len += n;
}
void cyx_array_print(const void* arr) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
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
void __cyx_array_sort(void* arr, int start, int end) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (end - start <= 1 || end < start) {
		if (end - start == 1 && (!head->is_ptr ?
			head->cmp_fn((char*)arr + start * head->size, (char*)arr + (start + 1) * head->size) == 1 :
			head->cmp_fn(*(void**)((char*)arr + start * head->size), *(void**)((char*)arr + (start + 1) * head->size)) == 1)) {
			__CYX_SWAP((char*)arr + start * head->size, (char*)arr + (start + 1) * head->size, head->size);
		}
		return;
	}
	void* pivot = (char*)arr + end * head->size;
	void* a = (char*)arr + (start - 1) * head->size;
	if (!head->is_ptr) {
		for (void* b = (char*)arr + start * head->size; b < pivot; b = (char*)b + head->size) {
			if (head->cmp_fn(b, pivot) == -1) {
				a = (char*)a + head->size;
				__CYX_SWAP(a, b, head->size);
			}
		}
	} else {
		for (void* b = (char*)arr + start * head->size; b < pivot; b = (char*)b + head->size) {
			if (head->cmp_fn(*(void**)b, *(void**)pivot) == -1) {
				a = (char*)a + head->size;
				__CYX_SWAP(a, b, head->size);
			}
		}
	}
	a = (char*)a + head->size;
	int mid = ((size_t)a - (size_t)arr) / head->size;
	__CYX_SWAP(a, pivot, head->size);

	__cyx_array_sort(arr, start, mid - 1);
	__cyx_array_sort(arr, mid + 1, end);
}
void* __cyx_array_map(const void* const arr, void (*fn)(void*, const void*)) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	void* curr = (void*)arr;
	void* res = __cyx_array_new((struct __CyxArrayParams){
		.__size = head->size,
		.reserve = head->cap,
		.defer_fn = head->defer_fn,
		.print_fn = head->print_fn,
		.cmp_fn = head->cmp_fn,
	});

	__cyx_temp_buffer_marker_set();
	void* curr_res = __cyx_temp_buffer_alloc(head->size);
	for (size_t i = 0; i < head->len; ++i) {
		fn(curr_res, curr);
		__cyx_array_append(&res, curr_res);
		curr = (char*)curr + head->size;
	}
	__cyx_temp_buffer_marker_reset();
	return res;
}
void* cyx_array_map_self(void* arr, void (*fn)(void*, const void*)) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	void* val = arr;

	__cyx_temp_buffer_marker_set();
	void* res = __cyx_temp_buffer_alloc(head->size);
	for (size_t i = 0; i < head->len; ++i) {
		fn(res, val);
		memcpy(val, res, head->size);
		val = (char*)val + head->size;
	}
	__cyx_temp_buffer_marker_reset();
	return arr;
}
void* __cyx_array_filter(const void* const arr, int (*fn)(const void*)) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	void* curr = (void*)arr;
	void* res = __cyx_array_new((struct __CyxArrayParams){
		.__size = head->size,
		.is_ptr = head->is_ptr,
		.reserve = head->cap,
		.defer_fn = head->defer_fn,
		.print_fn = head->print_fn,
		.cmp_fn = head->cmp_fn,
	});

	for (size_t i = 0; i < head->len; ++i) {
		if (fn(curr)) { __cyx_array_append(&res, curr); }
		curr = (char*)curr + head->size;
	}
	return res;
}
void* cyx_array_filter_self(void* arr, int (*fn)(const void*)) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
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
int cyx_array_find(void* arr, void* val) {
	assert(arr);
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	for (size_t i = 0; i < head->len; ++i) {
		if (!head->cmp_fn(val, (char*)arr + i * head->size)) {
			return (int)i;
		}
	}
	return -1;
}
int cyx_array_find_by(void* arr, int (*fn)(const void*)) {
	assert(arr);
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	for (size_t i = 0; i < head->len; ++i) {
		if (fn((char*)arr + i * head->size)) {
			return (int)i;
		}
	}
	return -1;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Bitmap
 */

#if __CYX_CLOSE_FOLD

#define cyx_bitmap_size(bitmap) (*(bitmap - 1))

size_t* cyx_bitmap_new(size_t size);
size_t* cyx_bitmap_copy(const size_t* const bitmap);
int cyx_bitmap_get(const size_t* const bitmap, int pos);
void cyx_bitmap_flip(size_t* bitmap, int pos);
void cyx_bitmap_set(size_t* bitmap, int pos, int val);
void cyx_bitmap_free(void* bitmap);
void cyx_bitmap_print(const void* bitmap);
size_t* cyx_bitmap_and(const size_t* const bitmap1, const size_t* const bitmap2);
size_t* cyx_bitmap_and_self(size_t* self, const size_t* const other);
size_t* cyx_bitmap_or(const size_t* const bitmap1, const size_t* const bitmap2);
size_t* cyx_bitmap_or_self(size_t* self, const size_t* const other);
size_t* cyx_bitmap_xor(const size_t* const bitmap1, const size_t* const bitmap2);
size_t* cyx_bitmap_xor_self(size_t* self, const size_t* const other);

#ifdef CYLIBX_STRIP_PREFIX

#define bitmap_size(bitmap) cyx_bitmap_size(bitmap)

#define bitmap_new cyx_bitmap_new
#define bitmap_copy cyx_bitmap_copy
#define bitmap_get cyx_bitmap_get
#define bitmap_flip cyx_bitmap_flip
#define bitmap_set cyx_bitmap_set
#define bitmap_free cyx_bitmap_free
#define bitmap_print cyx_bitmap_print

#define bitmap_and cyx_bitmap_and
#define bitmap_and_self cyx_bitmap_and_self
#define bitmap_or cyx_bitmap_or
#define bitmap_or_self cyx_bitmap_or_self
#define bitmap_xor cyx_bitmap_xor
#define bitmap_xor_self cyx_bitmap_xor_self

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

size_t* cyx_bitmap_new(size_t size) {
	size_t* ret = calloc(2 + (size - 1) / (8 * sizeof(size_t)), sizeof(size_t));
	if (!ret) { return NULL; }
	*ret = size;
	return ret + 1;
}
size_t* cyx_bitmap_copy(const size_t* const bitmap) {
	size_t* ret = malloc(((cyx_bitmap_size(bitmap) - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t));
	if (!ret) { return NULL; }
	*ret = cyx_bitmap_size(bitmap);
	memcpy(ret + 1, bitmap, (((*ret - 1) / (8 * sizeof(size_t)) + 1) * sizeof(size_t)));
	return ret + 1;
}
int cyx_bitmap_get(const size_t* const bitmap, int pos) {
	assert(bitmap);
	if (pos < 0) { pos += cyx_bitmap_size(bitmap); }
	assert(pos < (int)cyx_bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);
	return bitmap[bitmap_pos] & (0b1ull << inbyte_pos) ? 1 : 0;
}
void cyx_bitmap_flip(size_t* bitmap, int pos) {
	assert(bitmap);
	if (pos < 0) { pos += cyx_bitmap_size(bitmap); }
	assert(pos < (int)cyx_bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);
	if (bitmap[bitmap_pos] & (0b1 << inbyte_pos) ? 1 : 0) {
		bitmap[bitmap_pos] |= 0b1 << inbyte_pos;
	} else {
		bitmap[bitmap_pos] &= ~(0b1 << inbyte_pos);
	}
}
void cyx_bitmap_set(size_t* bitmap, int pos, int val) {
	assert(bitmap);
	if (pos < 0) { pos += cyx_bitmap_size(bitmap); }
	assert(pos < (int)cyx_bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);
	if (val) {
		bitmap[bitmap_pos] |= 0b1ull << inbyte_pos;
	} else {
		bitmap[bitmap_pos] &= ~(0b1ull << inbyte_pos);
	}
}
void cyx_bitmap_free(void* bitmap) {
	assert(bitmap);
	free((size_t*)bitmap - 1);
}
void cyx_bitmap_print(const void* bitmap) {
	printf("0b");
	for (size_t i = 0; i < cyx_bitmap_size((size_t*)bitmap); ++i) {
		printf("%c", cyx_bitmap_get(bitmap, i) + '0');
	}
}

size_t* cyx_bitmap_and(const size_t* const bitmap1, const size_t* const bitmap2) {
	if (!bitmap1 || !bitmap2 || cyx_bitmap_size(bitmap1) != cyx_bitmap_size(bitmap2)) { return NULL; }
	size_t* res = cyx_bitmap_copy(bitmap1);
	size_t size = (cyx_bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] &= bitmap2[i]; }
	return res;
}
size_t* cyx_bitmap_and_self(size_t* self, const size_t* const other) {
	if (!self || !other || cyx_bitmap_size(self) != cyx_bitmap_size(other)) { return NULL; }
	size_t size = (cyx_bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] &= other[i]; }
	return self;
}
size_t* cyx_bitmap_or(const size_t* const bitmap1, const size_t* const bitmap2) {
	if (!bitmap1 || !bitmap2 || cyx_bitmap_size(bitmap1) != cyx_bitmap_size(bitmap2)) { return NULL; }
	size_t* res = cyx_bitmap_copy(bitmap1);
	size_t size = (cyx_bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] |= bitmap2[i]; }
	return res;
}
size_t* cyx_bitmap_or_self(size_t* self, const size_t* const other) {
	if (!self || !other || cyx_bitmap_size(self) != cyx_bitmap_size(other)) { return NULL; }
	size_t size = (cyx_bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] |= other[i]; }
	return self;
}
size_t* cyx_bitmap_xor(const size_t* const bitmap1, const size_t* const bitmap2) {
	if (!bitmap1 || !bitmap2 || cyx_bitmap_size(bitmap1) != cyx_bitmap_size(bitmap2)) { return NULL; }
	size_t* res = cyx_bitmap_copy(bitmap1);
	size_t size = (cyx_bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] ^= bitmap2[i]; }
	return res;
}
size_t* cyx_bitmap_xor_self(size_t* self, const size_t* const other) {
	if (!self || !other || cyx_bitmap_size(self) != cyx_bitmap_size(other)) { return NULL; }
	size_t size = (cyx_bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] ^= other[i]; }
	return self;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Hash Functions
 */

#if __CYX_CLOSE_FOLD

size_t cyx_hash_size_t(const void* const val);
size_t cyx_hash_int(const void* const val);
size_t cyx_hash_double(const void* const val);
size_t cyx_hash_str(const void* const val);

#ifdef CYLIBX_STRIP_PREFIX

#define hash_size_t cyx_hash_size_t
#define hash_int cyx_hash_int
#define hash_double cyx_hash_double
#define hash_str cyx_hash_str

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

size_t cyx_hash_size_t(const void* const val) {
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
size_t cyx_hash_int(const void* const val) {
	int val_int = *(int*)val;
	size_t val_size = ((size_t)val_int << 32) | (size_t)val_int;
	return cyx_hash_size_t(&val_size);
}
size_t cyx_hash_double(const void* const val) {
	return cyx_hash_size_t(val);
}
size_t cyx_hash_str(const void* const val) {
    size_t hash = 5381;
	char* str = (char*)val;
	for (size_t i = 0; i < cyx_str_length(str); ++i) { 
        hash = ((hash << 5) + hash) + str[i];
	}
    return hash;
}

#endif // CYLIBX_IMPLEMENTATION
#endif // __CYX_CLOSE_FOLD

/*
 * HashSet
 */

#if __CYX_CLOSE_FOLD
typedef struct {
	size_t len;
	size_t cap;
	size_t size;
	
	char is_ptr;

	size_t (*hash_fn)(const void* const);
	int (*eq_fn)(const void* const, const void* const);
	void (*defer_fn)(void*);
	void (*print_fn)(const void* const);
} __CyxHashSetHeader;

struct __CyxHashSetParams {
	size_t __size;
	char is_ptr;

	size_t (*__hash_fn)(const void* const);
	int (*__eq_fn)(const void* const, const void* const);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
};
struct __CyxHashSetContainsParams {
	const void* const __set;
	void* __val;
	char defer;
};

#ifndef CYX_HASH_SET_BASE_SIZE
#define CYX_HASH_SET_BASE_SIZE 32
#endif // CYX_HASH_SET_BASE_SIZE

#define __CYX_HASH_SET_HEADER_SIZE (sizeof(__CyxHashSetHeader))
#define __CYX_HASH_SET_GET_HEADER(set) ((__CyxHashSetHeader*)set - 1)
#define __CYX_HASH_SET_GET_BITMAP(set) ((size_t*)((char*)set + __CYX_HASH_SET_GET_HEADER(set)->cap * __CYX_HASH_SET_GET_HEADER(set)->size) + 1)

void* __cyx_hashset_new(struct __CyxHashSetParams params);
void* __cyx_hashset_copy(const void* const set);
void __cyx_hashset_expand(void** set_ptr);
void __cyx_hashset_add(void** set_ptr, void* val);
void __cyx_hashset_add_mult_n(void** set_ptr, size_t n, void** mult);
void __cyx_hashset_remove(void* set, void* val);
int __cyx_hashset_contains(struct __CyxHashSetContainsParams params);
void cyx_hashset_free(void* set);
void cyx_hashset_print(const void* set);

void* __cyx_hashset_union(const void* const set1, const void* const set2);
void* __cyx_hashset_union(const void* const set1, const void* const set2);
void* __cyx_hashset_union_self(void* self, const void* const other);
void* __cyx_hashset_intersec(const void* const set1, const void* const set2);
void* __cyx_hashset_intersec_self(void* self, const void* const other);
void* __cyx_hashset_diff(const void* const set1, const void* const set2);
void* __cyx_hashset_diff_self(void* self, const void* const other);

#define cyx_hashset_length(set) (__CYX_HASH_SET_GET_HEADER(set)->len)
#define cyx_hashset_foreach(val, set) size_t __CYX_UNIQUE_VAL__(counter) = 0; \
	for (typeof(*set)* val = (set); \
		 __CYX_UNIQUE_VAL__(counter) < __CYX_HASH_SET_GET_HEADER(set)->cap; \
		 ++__CYX_UNIQUE_VAL__(counter), \
		 val = (char*)val + __CYX_HASH_SET_GET_HEADER(set)->size) \
		if (cyx_bitmap_get(__CYX_HASH_SET_GET_BITMAP(set), 2 * __CYX_UNIQUE_VAL__(counter)) && \
			!cyx_bitmap_get(__CYX_HASH_SET_GET_BITMAP(set), 2 * __CYX_UNIQUE_VAL__(counter) + 1))

#define __cyx_hashset_new_params(...) __cyx_hashset_new((struct __CyxHashSetParams){ 0, __VA_ARGS__ })
#define cyx_hashset_new(T, hash, eq, ...) (T*)__cyx_hashset_new_params(.__size = sizeof(T), .__hash_fn = hash, .__eq_fn = eq, __VA_ARGS__)
#define cyx_hashset_copy(set) (typeof(*set)*)__cyx_hashset_copy(set)
#define cyx_hashset_add(set, val) do { \
	typeof(*set) v = val; \
	__cyx_hashset_add((void**)&set, &v); \
} while(0)
#define cyx_hashset_add_mult_n(set, n, mult) __cyx_hashset_add_mult_n((void**)&(set), n, mult)
#define cyx_hashset_add_mult(set, ...) do { \
	typeof(*set) mult[] = { __VA_ARGS__ };\
	__cyx_hashset_add_mult_n((void**)&(set), n, mult); \
} while (0)
#define cyx_hashset_remove(set, val) do { \
	typeof(*set) v = val; \
	__cyx_hashset_remove(set, &v); \
} while(0)
#define __cyx_hashset_contains_params(...) __cyx_hashset_contains((struct __CyxHashSetContainsParams){ 0, __VA_ARGS__ })
#define cyx_hashset_contains(set, val, ...) ({ \
	typeof(*set) v = val; \
	__cyx_hashset_contains_params(.__set = set, .__val = &v, __VA_ARGS__); \
})

#define cyx_hashset_union(set1, set2) (typeof(*set1)*)__hashset_union(set1, set2)
#define cyx_hashset_union_self(self, other) (typeof(*self)*)__cyx_hashset_union_self(self, other)
#define cyx_hashset_intersec(set1, set2) (typeof(*set1)*)__cyx_hashset_intersec(set1, set2)
#define cyx_hashset_intersec_self(self, other) (typeof(*self)*)__cyx_hashset_intersec_self(self, other)
#define cyx_hashset_diff(set1, set2) (typeof(*set1)*)__cyx_hashset_diff(set1, set2)
#define cyx_hashset_diff_self(self, other) (typeof(*self)*)__cyx_hashset_diff_self(self, other)

#ifdef CYLIBX_STRIP_RREFIX

#define hashset_length(set) cyx_hashset_length(set)
#define hashset_foreach(val, set) cyx_hashset_foreach(val, set)

#define hashset_new(T, hash, eq, ...) cyx_hashset_new(T, hash, eq, __VA_ARGS__)
#define hashset_copy(set) cyx_hashset_copy(set)
#define hashset_add(set, val) cyx_hashset_add(set, val)
#define hashset_add_mult_n(set, n, mult) cyx_hashset_add_mult_n(set, n, mult)
#define hashset_add_mult(set, ...) cyx_hashset_add_mult(set, __VA_ARGS__)
#define hashset_remove(set, val) cyx_hashset_remove(set, val)
#define hashset_contains(set, val, ...) cyx_hashset_contains(set, val, __VA_ARGS__)

#define hashset_union(set1, set2) cyx_hashset_union(set1, set2)
#define hashset_union_self(self, other) cyx_hashset_union_self(self, other)
#define hashset_intersec(set1, set2) cyx_hashset_intersec(set1, set2)
#define hashset_intersec_self(self, other) cyx_hashset_intersec_self(self, other)
#define hashset_diff(set1, set2) cyx_hashset_diff(set1, set2)
#define hashset_diff_self(self, other) cyx_hashset_diff_self(self, other)

#define hashset_free cyx_hashset_free
#define hashset_print cyx_hashset_print

#endif // CYLIBX_STRIP_RREFIX


#ifdef CYLIBX_IMPLEMENTATION

void* __cyx_hashset_new(struct __CyxHashSetParams params) {
	size_t to_alloc = ((CYX_HASH_SET_BASE_SIZE * 2 - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t) + __CYX_HASH_SET_HEADER_SIZE + CYX_HASH_SET_BASE_SIZE * params.__size;
	void* ret = malloc(to_alloc);
	memset(ret, 0, to_alloc);

	__CyxHashSetHeader* head = ret;
	void* set = (char*)(head + 1);
	head->cap = CYX_HASH_SET_BASE_SIZE;
	head->size = params.__size;
	head->is_ptr = params.is_ptr;
	*(__CYX_HASH_SET_GET_BITMAP(set) - 1) = CYX_HASH_SET_BASE_SIZE << 1;
	head->hash_fn = params.__hash_fn;
	head->eq_fn = params.__eq_fn;
	head->print_fn = params.print_fn;
	head->defer_fn = params.defer_fn;

	return set;
}
void* __cyx_hashset_copy(const void* const set) {
	assert(set);
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);

	void* set_res = __cyx_hashset_new((struct __CyxHashSetParams){
		.__size = head->size,
		.__hash_fn = head->hash_fn,
		.__eq_fn = head->eq_fn,
		.is_ptr = head->is_ptr,
		.print_fn = head->print_fn,
		.defer_fn = head->defer_fn,
	});
	cyx_hashset_foreach(a, set) { __cyx_hashset_add(&set_res, (void*)a); }
	return set_res;
}
void __cyx_hashset_expand(void** set_ptr) {
	void* set = *set_ptr;
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	size_t to_alloc = __CYX_HASH_SET_HEADER_SIZE + (head->cap << 1) * head->size + (((head->cap << 2) - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t);
	void* ret = malloc(to_alloc);
	memset(ret, 0, to_alloc);

	__CyxHashSetHeader* new_head = ret;
	void* new_set = (void*)(new_head + 1);
	memcpy(new_head, head, __CYX_HASH_SET_HEADER_SIZE);
	new_head->cap <<= 1;

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	size_t* new_bitmap = __CYX_HASH_SET_GET_BITMAP(new_set);
	*(new_bitmap - 1) = 2 * new_head->cap;

	for (size_t i = 0; i < head->cap; ++i) {
		if (cyx_bitmap_get(bitmap, 2 * i)) {
			void* val = (char*)set + i * head->size;
			size_t pos = new_head->hash_fn(!new_head->is_ptr ? val : *(void**)val) % new_head->cap;
			for (size_t j = 0; j < new_head->cap; ++j) {
				size_t probe = (pos + j * j) % new_head->cap;
				if (!cyx_bitmap_get(new_bitmap, 2 * probe)) {
					memcpy((char*)new_set + probe * new_head->size, val, new_head->size);
					cyx_bitmap_set(new_bitmap, 2 * probe, 1);
					break;
				}
			}
		}
	}

	free(head);
	*set_ptr = new_set;
}
void __cyx_hashset_add(void** set_ptr, void* val) {
	void* set = *set_ptr;
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	if (head->len * 1000 > head->cap * 650) {
		__cyx_hashset_expand(set_ptr);
		set = *set_ptr;
		head = __CYX_HASH_SET_GET_HEADER(set);
	}
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!cyx_bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)set + probe * head->size, val, head->size);
			cyx_bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)set + probe * head->size, val, head->size);
			cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
			return;
		}
	}
}
void __cyx_hashset_add_mult_n(void** set_ptr, size_t n, void** mult) {
	void* set = *set_ptr;
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	while ((head->len + n) * 1000 > head->cap * 650) {
		__cyx_hashset_expand(set_ptr);
		set = *set_ptr;
		head = __CYX_HASH_SET_GET_HEADER(set);
	}
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	for (size_t nn = 0; nn < n; ++nn) {
		void* val = (char*)mult + nn * head->size;
		size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val) % head->cap;
		for (size_t i = 0; i < head->cap; ++i) {
			size_t probe = (pos + i * i) % head->cap;
			if (!cyx_bitmap_get(bitmap, 2 * probe)) {
				memcpy((char*)set + probe * head->size, val, head->size);
				cyx_bitmap_set(bitmap, 2 * probe, 1);
				++head->len;
				return;
			} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
				memcpy((char*)set + probe * head->size, val, head->size);
				cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
				++head->len;
				return;
			} else if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
				return;
			}
		}
	}
}
void __cyx_hashset_remove(void* set, void* val) {
	assert(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val);
	char found = -1;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (cyx_bitmap_get(bitmap, 2 * probe)) {
			if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
				found = (int)probe;
			}
		}
	}
	if (found != -1) {
		cyx_bitmap_set(bitmap, 2 * found + 1, 1);
		if (head->defer_fn) {
			if (!head->is_ptr) {
				head->defer_fn(__CYX_DATA_GET_AT(head, set, found));
				head->defer_fn(val);
			} else {
				head->defer_fn(*(void**)__CYX_DATA_GET_AT(head, set, found));
				head->defer_fn(*(void**)val);
			}
		}
		--head->len;
	}
}
int __cyx_hashset_contains(struct __CyxHashSetContainsParams params) {
	void* set = (void*)params.__set;
	void* val = params.__val;
	assert(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);

	int res = 0;
	for (size_t i = 0; i < head->cap; ++i) {
		if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
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
void cyx_hashset_free(void* set) {
	assert(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	if (head->defer_fn) {
		if (!head->is_ptr) {
			cyx_hashset_foreach(a, set) { head->defer_fn(a); }
		} else {
			cyx_hashset_foreach(a, set) { head->defer_fn(*(void**)a); }
		}
	}
	
	free(head);
}
void cyx_hashset_print(const void* set) {
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	printf("{ ");
	char started = 0;
	if (!head->is_ptr) {
		cyx_hashset_foreach(a, set) {
			if (started) { printf(", "); } else { started = 1; }
			head->print_fn(a);
		}
	} else {
		cyx_hashset_foreach(a, set) {
			if (started) { printf(", "); } else { started = 1; }
			head->print_fn(*(void**)a);
		}
	}
	printf(" }");
}
void* __cyx_hashset_union(const void* const set1, const void* const set2) {
	assert(set1 && set2);
	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(set1);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __cyx_hashset_copy(set1);
	cyx_hashset_foreach(a, set2) { __cyx_hashset_add(&set_res, (void*)a); }

	return set_res;
}
void* __cyx_hashset_union_self(void* self, const void* const other) {
	assert(self && other);
	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(self);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(other);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	cyx_hashset_foreach(a, other) { __cyx_hashset_add(&self, (void*)a); }

	return self;
}
void* __cyx_hashset_intersec(const void* const set1, const void* const set2) {
	assert(set1 && set2);
	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(set1);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __cyx_hashset_copy(set1);
	cyx_hashset_foreach(a, set1) {
		if (!__cyx_hashset_contains_params(.__set = set2, .__val = (void*)a, .defer = 0)) {
			__cyx_hashset_remove(set_res, (void*)a);
		}
	}

	return set_res;
}
void* __cyx_hashset_intersec_self(void* self, const void* const other) {
	assert(self && other);
	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(self);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(other);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	cyx_hashset_foreach(a, other) {
		if (!__cyx_hashset_contains_params(.__set = other, .__val = (void*)a, .defer = 0)) {
			__cyx_hashset_remove(self, (void*)a);
		}
	}

	return self;
}
void* __cyx_hashset_diff(const void* const set1, const void* const set2) {
	assert(set1 && set2);
	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(set1);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __cyx_hashset_copy(set1);
	cyx_hashset_foreach(a, set2) { __cyx_hashset_remove(set_res, (void*)a); }

	return set_res;
}
void* __cyx_hashset_diff_self(void* self, const void* const other) {
	assert(self && other);
	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(self);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(other);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	cyx_hashset_foreach(a, other) { __cyx_hashset_remove(self, (void*)a); }

	return self;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * HashMap
 */

#if __CYX_CLOSE_FOLD

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
} __CyxHashMapHeader;

struct __CyxHashMapParams {
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
struct __CyxHashMapFuncParams {
	void* __map;
	void* __key;
	char defer;
};

#ifndef CYX_HASHMAP_BASE_SIZE
#define CYX_HASHMAP_BASE_SIZE 32
#endif // CYX_HASHMAP_BASE_SIZE

#define __CYX_HASHMAP_HEADER_SIZE (sizeof(__CyxHashMapHeader))
#define __CYX_HASHMAP_GET_HEADER(map) ((__CyxHashMapHeader*)(map) - 1)
#define __CYX_HASHMAP_GET_BITMAP(map) ((size_t*)((char*)(map) + __CYX_HASHMAP_GET_HEADER(map)->size * __CYX_HASHMAP_GET_HEADER(map)->cap) + 1)

void* __cyx_hashmap_new(struct __CyxHashMapParams params);
void __cyx_hashmap_expand(void** map_ptr);
void __cyx_hashmap_add(void** map_ptr, void* key);
void __cyx_hashmap_add_v(void** map_ptr, void* key, void* val);
void* __cyx_hashmap_get(struct __CyxHashMapFuncParams params);
void __cyx_hashmap_remove(struct __CyxHashMapFuncParams params);
void cyx_hashmap_free(void* map);
void cyx_hashmap_print(const void* map);

#define cyx_hashmap_size(map) (__CYX_HASHMAP_GET_HEADER(map)->len)
#define cyx_hashmap_foreach(val, map) size_t __CYX_UNIQUE_VAL__(i) = 0; \
	for (typeof(map->value)* val = (void*)((char*)map + __CYX_HASHMAP_GET_HEADER(map)->size_key); \
		__CYX_UNIQUE_VAL__(i) < __CYX_HASHMAP_GET_HEADER(map)->cap; \
		++__CYX_UNIQUE_VAL__(i), \
		val = (typeof(map->value)*)((char*)val + __CYX_HASHMAP_GET_HEADER(map)->size)) \
		if (cyx_bitmap_get(__CYX_HASHMAP_GET_BITMAP(map), 2 * __CYX_UNIQUE_VAL__(i)) && \
			!cyx_bitmap_get(__CYX_HASHMAP_GET_BITMAP(map), 2 * __CYX_UNIQUE_VAL__(i) + 1))

#define __cyx_hashmap_new_params(...) __cyx_hashmap_new((struct __CyxHashMapParams){ 0, __VA_ARGS__ })
#define cyx_hashmap_new(T, hash, eq, ...) (T*)__cyx_hashmap_new_params(.__size_key = sizeof((T){0}.key), .__size_value = sizeof((T){0}.value), .__hash_fn = hash, .__eq_fn = eq, __VA_ARGS__)
#define cyx_hashmap_add(map, k) do { \
	typeof(map->key) key = k; \
	__cyx_hashmap_add((void**)&(map), &key); \
} while(0)
#define cyx_hashmap_add_v(map, k, v) do { \
	typeof(map->key) key = k; \
	typeof(map->value) val = v; \
	__cyx_hashmap_add_v((void**)&(map), &key, &val); \
} while(0)
#define __cyx_hashmap_get_params(...) __cyx_hashmap_get((struct __HashMapFuncParams){ 0, __VA_ARGS__ })
#define cyx_hashmap_get(map, k, ...) ({ \
	typeof(map->key) key = k; \
	(typeof(map->value)*) __cyx_hashmap_get_params( .__map = map, .__key = &key, __VA_ARGS__ ); \
})
#define __cyx_hashmap_remove_params(...) __cyx_hashmap_remove((struct __HashMapFuncParams){ 0, __VA_ARGS__ })
#define cyx_hashmap_remove(map, k, ...) do { \
	typeof(map->key) key = k; \
	__cyx_hashmap_remove_params( .__map = map, .__key = &key, __VA_ARGS__ ); \
} while(0)

// #ifdef CYLIBX_STRIP_PREFIX

#define hashmap_size(map) cyx_hashmap_size(map)
#define hashmap_foreach(val, map) cyx_hashmap_foreach(val, map)

#define hashmap_new(T, hash, eq, ...) cyx_hashmap_new(T, hash, eq, __VA_ARGS__)
#define hashmap_add(map, k) cyx_hashmap_add(map, k)
#define hashmap_add_v(map, k, v) cyx_hashmap_add_v(map, k, v)
#define hashmap_get(map, k, ...) cyx_hashmap_get(map, k, __VA_ARGS__)
#define hashmap_remove(map, k, ...) cyx_hashmap_remove(map, k, ...)

#define hashmap_free cyx_hashmap_free
#define hashmap_print cyx_hashmap_print

// #endif // CYLIBX_STRIP_PREFIX


#ifdef CYLIBX_IMPLEMENTATION

void* __cyx_hashmap_new(struct __CyxHashMapParams params) {
	size_t alloc_size = __CYX_HASHMAP_HEADER_SIZE +
			CYX_HASHMAP_BASE_SIZE * (params.__size_key + params.__size_value) +
			((2 * CYX_HASHMAP_BASE_SIZE - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t);
	__CyxHashMapHeader* head = malloc(alloc_size);
	memset(head, 0, alloc_size);

	head->len = 0;
	head->cap = CYX_HASHMAP_BASE_SIZE;
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
	cyx_bitmap_size(__CYX_HASHMAP_GET_BITMAP(map)) = 2 * CYX_HASHMAP_BASE_SIZE;
	return map;
}
void __cyx_hashmap_expand(void** map_ptr) {
	void* map = *map_ptr;
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);

	size_t alloc_size = __CYX_HASHMAP_HEADER_SIZE + 
			(head->cap << 1) * head->size +
			(((head->cap << 2) - 1) / (8 * sizeof(size_t)) + 2) * sizeof(size_t);
	__CyxHashMapHeader* new_head = malloc(alloc_size);

	memset(new_head, 0, alloc_size);
	memcpy(new_head, head, __CYX_HASHMAP_HEADER_SIZE);
	new_head->len = 0;
	new_head->cap <<= 1;
	void* new_map = new_head + 1;
	size_t* new_bitmap = __CYX_HASHMAP_GET_BITMAP(new_map);
	cyx_bitmap_size(new_bitmap) = new_head->cap << 1;
	
	for (size_t i = 0; i < head->cap; ++i) {
		if (!cyx_bitmap_get(bitmap, 2 * i) || cyx_bitmap_get(bitmap, 2 * i + 1)) { continue; }

		void* key = (char*)map + i * head->size;
		size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % new_head->cap;
		for (size_t j = 0; j < new_head->cap; ++j) {
			size_t probe = (pos + j * j) % new_head->cap;
			if (!cyx_bitmap_get(new_bitmap, 2 * probe)) {
				memcpy((char*)new_map + probe * head->size, key, head->size);
				cyx_bitmap_set(new_bitmap, 2 * probe, 1);
				++head->len;
				break;
			} else if (cyx_bitmap_get(new_bitmap, 2 * probe + 1)) {
				memcpy((char*)new_map + probe * head->size, key, head->size);
				cyx_bitmap_set(new_bitmap, 2 * probe + 1, 0);
				++new_head->len;
				break;
			}
		}
	}

	free(head);
	*map_ptr = new_map;
}
void __cyx_hashmap_add(void** map_ptr, void* key) {
	void* map = *map_ptr;
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	if (head->len * 1000 >= head->cap * 650) {
		__cyx_hashmap_expand(map_ptr);
		map = *map_ptr;
		head = __CYX_HASHMAP_GET_HEADER(map);
	}

	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!cyx_bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			cyx_bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_key_ptr ? head->eq_fn((char*)map + probe * head->size, key) : head->eq_fn(*(void**)((char*)map + probe * head->size), *(void**)key)) {
			return;
		}
	}
}
void __cyx_hashmap_add_v(void** map_ptr, void* key, void* val) {
	void* map = *map_ptr;
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	if (head->len * 1000 >= head->cap * 650) {
		__cyx_hashmap_expand(map_ptr);
		map = *map_ptr;
		head = __CYX_HASHMAP_GET_HEADER(map);
	}

	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!cyx_bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			cyx_bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_key_ptr ? head->eq_fn((char*)map + probe * head->size, key) : head->eq_fn(*(void**)((char*)map + probe * head->size), *(void**)key)) {
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			return;
		}

	}
}
void* __cyx_hashmap_get(struct __CyxHashMapFuncParams params) {
	assert(params.__map);
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(params.__map);
	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(params.__map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key) % head->cap;

	void* res = NULL;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (cyx_bitmap_get(bitmap, 2 * probe) && !cyx_bitmap_get(bitmap, 2 * probe + 1)) {
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
void __cyx_hashmap_remove(struct __CyxHashMapFuncParams params) {
	assert(params.__map);
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(params.__map);
	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(params.__map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key) % head->cap;

	int res = -1;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (cyx_bitmap_get(bitmap, 2 * probe) && !cyx_bitmap_get(bitmap, 2 * probe + 1)) {
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
		cyx_bitmap_set(bitmap, 2 * res + 1, 1);
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
void cyx_hashmap_free(void* map) {
	assert(map);
	
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	if (head->defer_key_fn) {
		if (!head->is_key_ptr) {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_key_fn((char*)map + i * head->size);
				}
			}
		} else {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_key_fn(*(void**)((char*)map + i * head->size));
				}
			}
		}
	}

	if (head->defer_value_fn) {
		if (!head->is_value_ptr) {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_value_fn((char*)map + i * head->size + head->size_key);
				}
			}
		} else {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					void* val = *(void**)((char*)map + i * head->size + head->size_key);
					if (val) {
						head->defer_value_fn(val);
					}
				}
			}
		}
	}
	free(head);
}
void cyx_hashmap_print(const void* map) {
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	assert(head->print_key_fn && head->print_value_fn);
	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);

	printf("{ ");
	char started = 0;
	for (size_t i = 0; i < head->cap; ++i) {
		if (!cyx_bitmap_get(bitmap, 2 * i) || cyx_bitmap_get(bitmap, 2 * i + 1)) { continue; }

		if (started) { printf(", "); } else { started = 1; }
		void* key = (char*)map + i * head->size;
		void* val = (char*)map + i * head->size + head->size_key;
		head->print_key_fn(!head->is_key_ptr ? key : *(void**)key);
		printf(" : ");
		head->print_value_fn(!head->is_value_ptr ? val : *(void**)val);
	}
	printf(" }");
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * BinaryHeap
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
	size_t size;

	char is_ptr;

	int (*cmp_fn)(const void*, const void*);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
} __CyxBinaryHeapHeader;

struct __CyxBinaryHeapParams {
	size_t __size;

	char is_ptr;

	int (*__cmp_fn)(const void*, const void*);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
};
struct __CyxBinaryHeapSearchParams {
	void* __heap;
	void* __val;
	char defer;
};

#ifndef CYX_BINHEAP_BASE_SIZE
#define CYX_BINHEAP_BASE_SIZE 16
#endif // CYX_BINHEAP_BASE_SIZE

#define __CYX_BINHEAP_HEADER_SIZE (sizeof(__CyxBinaryHeapHeader))
#define __CYX_BINHEAP_GET_HEADER(heap) ((__CyxBinaryHeapHeader*)(heap) - 1)

void* __cyx_binheap_new(struct __CyxBinaryHeapParams params);
void __cyx_binheap_expand(void** heap_ptr, size_t n);
void __cyx_binheap_insert(void** heap_ptr, void* val);
void __cyx_binheap_insert_mult_n(void** heap_ptr, size_t n, void* mult);
void __cyx_binheap_try_fit(void* heap, size_t curr);
void* __cyx_binheap_extract(void* heap);
int __cyx_binheap_contains(struct __CyxBinaryHeapSearchParams params);
int __cyx_binheap_remove(struct __CyxBinaryHeapSearchParams params);
void cyx_binheap_free(void* heap);
void cyx_binheap_print(const void* heap);

#define cyx_binheap_length(heap) (__CYX_BINHEAP_GET_HEADER(heap)->len)
#define cyx_binheap_drain(val, heap) for (typeof(*heap)* val = NULL; (val = cyx_binheap_extract(heap));)

#define __cyx_binheap_new_params(...) __cyx_binheap_new((struct __CyxBinaryHeapParams){ 0, __VA_ARGS__ })
#define cyx_binheap_new(T, cmp, ...) (T*)__cyx_binheap_new_params(.__size = sizeof(T), .__cmp_fn = cmp, __VA_ARGS__)
#define cyx_binheap_insert(heap, val) do { \
	typeof(*heap) v = val; \
	__cyx_binheap_insert((void**)&(heap), &v); \
} while(0)
#define cyx_binheap_insert_mult_n(heap, n, mult) __cyx_binheap_insert_mult_n((void**)&(heap), n, mult)
#define cyx_binheap_insert_mult(heap, ...) do { \
	typeof(*heap) mult[] = { __VA_ARGS__ }; \
	__cyx_binheap_insert_mult_n((void**)&(heap), sizeof(mult)/sizeof(*mult), mult); \
} while(0)
#define cyx_binheap_extract(heap) (typeof(*heap)*)__cyx_binheap_extract(heap)
#define __cyx_binheap_contains_params(...) __cyx_binary_contains((struct __CyxBinaryHeapSearchParams){ 0, __VA_ARGS__ })
#define cyx_binheap_contains(heap, val, ...) ({ \
	typeof(*(heap)) v = (val); \
	__cyx_binary_contains_params(.__heap = (heap), .__val = &v, __VA_ARGS__); \
})
#define __cyx_binheap_remove_params(...) __cyx_binheap_remove((struct __CyxBinaryHeapSearchParams){ 0, __VA_ARGS__ })
#define cyx_binheap_remove(heap, val, ...) ({ \
	typeof(*heap) v = (val); \
	__cyx_binheap_remove_params(.__heap = (heap), .__val = &v, __VA_ARGS__); \
})

#ifdef CYLIBX_STRIP_PREFIX

#define binheap_length(heap) cyx_binheap_length(heap)
#define binheap_drain(val, heap) cyx_binheap_drain(val, heap)

#define binheap_new(T, cmp, ...) cyx_binheap_new(T, cmp, __VA_ARGS__)
#define binheap_insert(heap, val) cyx_binheap_insert(heap, val)
#define binheap_insert_mult_n(heap, n, mult) cyx_binheap_insert_mult_n(heap, n, mult)
#define binheap_insert_mult(heap, ...) cyx_binheap_insert_mult(heap, __VA_ARGS__)
#define binheap_extract(heap) cyx_binheap_extract(heap)
#define binheap_contains(heap, val, ...) cyx_binheap_contains(heap, val, __VA_ARGS__)
#define binheap_remove(heap, val, ...) cyx_binheap_remove(heap, val, __VA_ARGS__)

#define binheap_free cyx_binheap_free
#define binheap_print cyx_binheap_print

#endif // CYLIBX_STRIP_RREFIX

#ifdef CYLIBX_IMPLEMENTATION

#define __cyx_get_left_child(n) (((n) << 1) + 1)
#define __cyx_get_right_child(n) (((n) << 1) + 2)
#define __cyx_get_parent(n) (((n) - 1) >> 1)

void* __cyx_binheap_new(struct __CyxBinaryHeapParams params) {
	__CyxBinaryHeapHeader* head = malloc(__CYX_BINHEAP_HEADER_SIZE + params.__size * CYX_BINHEAP_BASE_SIZE);
	assert(head);
	void* heap = head + 1;

	head->len = 0;
	head->cap = CYX_BINHEAP_BASE_SIZE;
	head->size = params.__size;
	head->is_ptr = params.is_ptr;

	head->cmp_fn = params.__cmp_fn;
	head->defer_fn = params.defer_fn;
	head->print_fn = params.print_fn;

	return heap;
}
void __cyx_binheap_expand(void** heap_ptr, size_t n) {
	void* heap = *heap_ptr;
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	size_t new_cap = head->cap;
	while (new_cap < head->len + n) { new_cap <<= 1; }

	__CyxBinaryHeapHeader* new_head = malloc(__CYX_BINHEAP_HEADER_SIZE + new_cap * head->size);
	new_head->len = head->len;
	new_head->cap = new_cap;
	new_head->size = head->size;
	new_head->is_ptr = head->is_ptr;

	new_head->cmp_fn = head->cmp_fn;
	new_head->defer_fn = head->defer_fn;
	new_head->print_fn = head->print_fn;

	void* new_heap = new_head + 1;
	memcpy(new_heap, heap, head->cap * head->size);
	free(head);
	*heap_ptr = new_heap;
}
void __cyx_binheap_insert(void** heap_ptr, void* val) {
	void* heap = *heap_ptr;
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	if (head->cap < head->len + 1) {
		__cyx_binheap_expand(heap_ptr, 1);
		heap = *heap_ptr;
		head = __CYX_BINHEAP_GET_HEADER(heap);
	}

	size_t pos = head->len++;
	size_t parent_pos = __cyx_get_parent(pos);;
	while (pos) {
		if (__CYX_PTR_CMP(head, val, (char*)heap + parent_pos * head->size) != -1) { break; }
		memcpy(__CYX_DATA_GET_AT(head, heap, pos), __CYX_DATA_GET_AT(head, heap, parent_pos), head->size);
		pos = parent_pos;
		parent_pos = __cyx_get_parent(pos);
	}
	memcpy(__CYX_DATA_GET_AT(head, heap, pos), val, head->size);
}
void __cyx_binheap_insert_mult_n(void** heap_ptr, size_t n, void* mult) {
	void* heap = *heap_ptr;
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	if (head->cap < n + head->len) {
		__cyx_binheap_expand(heap_ptr, n);
		heap = *heap_ptr;
		head = __CYX_BINHEAP_GET_HEADER(heap);
	}

	for (size_t i = 0; i < n; ++i) {
		__cyx_binheap_insert(heap_ptr, __CYX_DATA_GET_AT(head, mult, i));
	}
}
void __cyx_binheap_try_fit(void* heap, size_t curr) {
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	while (curr < head->len) {
		size_t left = __cyx_get_left_child(curr);
		if (left >= head->len) { break; }
		size_t right = __cyx_get_right_child(curr);
		if (right < head->len) {
			if (__CYX_PTR_CMP_POS(head, heap, left, right) == -1) {
				if (__CYX_PTR_CMP_POS(head, heap, curr, left) == 1) {
					__CYX_SWAP(__CYX_DATA_GET_AT(head, heap, curr), __CYX_DATA_GET_AT(head, heap, left), head->size);
					curr = left;
				} else { break; }
			} else {
				if (__CYX_PTR_CMP_POS(head, heap, curr, right) == 1) {
					__CYX_SWAP(__CYX_DATA_GET_AT(head, heap, curr), __CYX_DATA_GET_AT(head, heap, right), head->size);
					curr = right;
				} else { break; }
			} 
		} else {
			if (__CYX_PTR_CMP_POS(head, heap, curr, left) == 1) {
				__CYX_SWAP(__CYX_DATA_GET_AT(head, heap, curr), __CYX_DATA_GET_AT(head, heap, left), head->size);
				curr = left;
			} else { break; }
		}
	}
}
void* __cyx_binheap_extract(void* heap) {
	assert(heap);
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	if (!head->len) { return NULL; }

	--head->len;
	__CYX_SWAP(heap, __CYX_DATA_GET_AT(head, heap, head->len), head->size);
	__cyx_binheap_try_fit(heap, 0);

	return __CYX_DATA_GET_AT(head, heap, head->len);
}
int __cyx_binheap_contains(struct __CyxBinaryHeapSearchParams params) {
	assert(params.__heap);
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(params.__heap);

	int res = 0;
	for (size_t i = 0; i < head->len; ++i) {
		if (__CYX_PTR_CMP(head, params.__val, __CYX_DATA_GET_AT(head, params.__heap, i)) == 0) {
			res = 1;
			break;
		}
	}
	if (params.defer && head->defer_fn) {
		head->defer_fn(!head->is_ptr ? params.__val : *(void**)params.__val);
	}
	return res;
}
int __cyx_binheap_remove(struct __CyxBinaryHeapSearchParams params) {
	assert(params.__heap);
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(params.__heap);

	int res = -1;
	for (size_t i = 0; i < head->len; ++i) {
		if (__CYX_PTR_CMP(head, params.__val, __CYX_DATA_GET_AT(head, params.__heap, i)) == 0) {
			res = (int)i;
			break;
		}
	}

	if (head->defer_fn && params.defer) {
		head->defer_fn(!head->is_ptr ? params.__val : *(void**)params.__val);
	}
	if (res != -1) {
		if (head->defer_fn) {
			head->defer_fn(!head->is_ptr ? __CYX_DATA_GET_AT(head, params.__heap, res) : *(void**)__CYX_DATA_GET_AT(head, params.__heap, res));
		}
		--head->len;
		memcpy(__CYX_DATA_GET_AT(head, params.__heap, res), __CYX_DATA_GET_AT(head, params.__heap, head->len), head->size);
		__cyx_binheap_try_fit(params.__heap, res);
	}
	return res != -1;
}
void cyx_binheap_free(void* heap) {
	assert(heap);
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);

	if (head->defer_fn) {
		if (!head->is_ptr) {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn(__CYX_DATA_GET_AT(head, heap, i));
			} 
		} else {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn(*(void**)__CYX_DATA_GET_AT(head, heap, i));
			}
		}
	}


	free(head);
}
void cyx_binheap_print(const void* heap) {
	assert(heap);
	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	assert(head->print_fn);

	if (!head->len) { 
		printf("{ }");
		return;
	}
	size_t layer_count = 0;
	for (size_t pos = head->len; pos << 1; pos >>= 1) { ++layer_count; }

	size_t curr_pos = 0;
	printf("{ ");
	for (size_t i = 0; i < layer_count; ++i) {
		if (i) { printf(", { "); } else { printf("{ "); }
		for (size_t j = 0; j < (0b1ull << i); ++j) {
			if (curr_pos >= head->len) { break; }
			if (j) { printf(", "); }
			head->print_fn(!head->is_ptr ? (char*)heap + curr_pos * head->size : *(void**)((char*)heap + curr_pos * head->size));
			curr_pos++;
		}
		printf(" }");
	}
	printf(" }");
}

#undef __cyx_get_right_child
#undef __cyx_get_left_child
#undef __cyx_get_parent

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * RingBuffer
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
	size_t size;

	size_t start;
	size_t end;

	char is_ptr;

	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
} __CyxRingBufHeader;

struct __CyxRingBufParams {
	size_t __size;

	char is_ptr;
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
};

#ifndef CYX_RINGBUF_BASE_SIZE
#define CYX_RINGBUF_BASE_SIZE (CYX_ARRAY_BASE_SIZE)
#endif // CYX_RINGBUF_BASE_SIZE

#define __CYX_RINGBUF_HEADER_SIZE (sizeof(__CyxRingBufHeader))
#define __CYX_RINGBUF_GET_HEADER(ring) ((__CyxRingBufHeader*)(ring) - 1)

void* __cyx_ring_new(struct __CyxRingBufParams params);
void __cyx_ring_expand(void** ring_ptr, size_t n);
void __cyx_ring_push(void** ring_ptr, void* val);
void __cyx_ring_push(void** ring_ptr, void* val);
void __cyx_ring_push_mult_n(void** ring_ptr, size_t n, void** mult);
void* __cyx_ring_pop(void* ring);
void cyx_ring_free(void* ring);
void cyx_ring_print(const void* ring);

#define cyx_ring_length(ring) (__CYX_RINGBUF_GET_HEADER(ring)->len)
#define cyx_ring_foreach(val, ring) __CyxRingBufHeader* __CYX_UNIQUE_VAL__(head) = __CYX_RINGBUF_GET_HEADER(ring); \
	char __CYX_UNIQUE_VAL__(started) = __CYX_UNIQUE_VAL__(head)->len != 0; \
	size_t __CYX_UNIQUE_VAL__(i) = __CYX_UNIQUE_VAL__(head)->start; \
	for (typeof(*ring)* val = __CYX_DATA_GET_AT(__CYX_UNIQUE_VAL__(head), ring, __CYX_UNIQUE_VAL__(head)->start);\
		__CYX_UNIQUE_VAL__(i) != __CYX_UNIQUE_VAL__(head)->end || __CYX_UNIQUE_VAL__(started); \
		__CYX_UNIQUE_VAL__(i) = (__CYX_UNIQUE_VAL__(i) + 1) % __CYX_UNIQUE_VAL__(head)->cap, \
		val = (char*)val + __CYX_UNIQUE_VAL__(head)->size, \
		__CYX_UNIQUE_VAL__(started) = 0)
#define cyx_ring_drain(val, ring) for(typeof(*ring)* val = NULL; (val = cyx_ring_pop(ring));)
#define __cyx_ring_new_params(...) __cyx_ring_new((struct __RingBufParams){ __VA_ARGS__ })
#define cyx_ring_new(T, ...) (T*)__cyx_ring_new_params(.__size = sizeof(T), __VA_ARGS__)
#define cyx_ring_push(ring, val) do { \
	typeof(*ring) v = (val); \
	__cyx_ring_push((void**)&(ring), &v); \
} while(0)
#define cyx_ring_push_mult_n(ring, n, mult) __cyx_ring_push_mult_n((void**)&(ring), n, mult);
#define cyx_ring_push_mult(ring, ...) do { \
	typeof(*ring) mult[] = { __VA_ARGS__ }; \
	cyx_ring_push_mult_n(ring, sizeof(*mult)/sizeof(mult), mult); \
} while (0)
#define cyx_ring_pop(ring) (typeof(*ring)*)__cyx_ring_pop(ring)


#ifdef CYLIBX_STRIP_PREFFIX

#define ring_length(ring) cyx_ring_length(ring)
#define ring_foreach(val, ring) cyx_ring_foreach(val, ring)
#define ring_drain(val, ring) cyx_ring_drain(val, ring)

#define ring_new(T, ...) cyx_ring_new(T, __VA_ARGS__)
#define ring_push(ring, val) cyx_ring_push(ring, val)
#define ring_push_mult_n(ring, n, mult) cyx_ring_push_mult_n(ring, n, mult)
#define ring_push_mult(ring, ...) cyx_ring_push_mult(ring, __VA_ARGS__)
#define ring_pop(ring) cyx_ring_pop(ring)

#define ring_free cyx_ring_free
#define ring_print cyx_ring_print

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION 

void* __cyx_ring_new(struct __CyxRingBufParams params) {
	__CyxRingBufHeader* head = malloc(__CYX_RINGBUF_HEADER_SIZE + params.__size * CYX_RINGBUF_BASE_SIZE);
	memset(head, 0, __CYX_RINGBUF_HEADER_SIZE + params.__size * CYX_RINGBUF_BASE_SIZE);
	head->size = params.__size;
	head->cap = CYX_RINGBUF_BASE_SIZE;
	head->len = 0;

	head->start = 0;
	head->end = 0;

	head->is_ptr = params.is_ptr;
	head->defer_fn = params.defer_fn;
	head->print_fn = params.print_fn;

	return head + 1;
}
void __cyx_ring_expand(void** ring_ptr, size_t n) {
	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);

	size_t new_cap = head->cap;
	while ((new_cap <<= 1) <= head->len + n);

	__CyxRingBufHeader* new_head = malloc(__CYX_RINGBUF_HEADER_SIZE + new_cap * head->size);
	void* new_ring = new_head + 1;
	memcpy(new_head, head, __CYX_RINGBUF_HEADER_SIZE);
	new_head->cap = new_cap;
	new_head->start = 0;
	new_head->end = 0;

	cyx_ring_foreach(val, *ring_ptr) {
		memcpy(__CYX_DATA_GET_AT(new_head, new_ring, new_head->len), val, head->size);
		new_head->len++;
		new_head->end++;
	}

	free(head);
	*ring_ptr = new_ring;
}
void __cyx_ring_push(void** ring_ptr, void* val) {
	assert(*ring_ptr);
	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	if (head->len == head->cap) {
		__cyx_ring_expand(ring_ptr, 1);
		head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	}

	if (head->len == 0) { head->end = 0; head->start = 0; }
	memcpy(__CYX_DATA_GET_AT(head, *ring_ptr, head->end++), val, head->size);
	if (head->end == head->cap) { head->end = 0; }

	head->len++;
}
void __cyx_ring_push_mult_n(void** ring_ptr, size_t n, void** mult) {
	assert(*ring_ptr);
	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	if (head->len + n >= head->cap) {
		__cyx_ring_expand(ring_ptr, n);
		head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	}

	for (size_t i = 0; i < n; ++i) {
		if (head->len == 0) { head->end = 0; head->start = 0; }
		memcpy(__CYX_DATA_GET_AT(head, *ring_ptr, head->end), __CYX_DATA_GET_AT(head, mult, head->end), head->size);
		head->end++;
		if (head->end == head->cap) { head->end = 0; }

		head->len++;
	}
}
void* __cyx_ring_pop(void* ring) {
	assert(ring);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(ring);
	if (!head->len) { return NULL; }
	void* ret = __cyx_temp_alloc_deleted(head->size, __CYX_DATA_GET_AT(head, ring, head->start), head->is_ptr, head->defer_fn);
	++head->start;
	--head->len;
	if (head->start == head->cap) { head->start = 0; }

	return ret;
}
void cyx_ring_free(void* ring) {
	assert(ring);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(ring);
	if (head->defer_fn) {
		if (!head->is_ptr) {
			cyx_ring_foreach(val, ring) {
				head->defer_fn(val);
			}
		} else {
			cyx_ring_foreach(val, ring) {
				head->defer_fn(*(void**)val);
			}
		}
	}
	free(head);
}
void cyx_ring_print(const void* ring) {
	assert(ring);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(ring);
	printf("[ ");
	char started = 0;
	cyx_ring_foreach(val, ring) {
		if (started) { printf(", "); } else { started = 1; }
		head->print_fn(!head->is_ptr ? val : *(void**)val);
	}
	printf(" ]");
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

#endif // __CYLIBX_H__
