#define CYLIBX_STRIP_PREFIX
#include "cylibx.h"

// additional functions for examples
int int_compare(const void* a, const void* b) {
	int aa = *(int*)a;
	int bb = *(int*)b;
	return aa < bb ? -1 : (aa > bb ? 1 : 0);
}
int int_compare_op(const void* a, const void* b) { return int_compare(a, b) * -1; }
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
typedef struct { char* key; int* value; } KV2;

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

		// mult append
		int* mult_arr = array_new(int, .print_fn = int_print);
		array_append_mult(mult_arr, 1, 2, 3, 4, 5);
		array_print(mult_arr);
		putchar('\n');
		array_free(mult_arr);
	}

	// string example
	printf("\nString examples:\n"); {
		char* str = str_from_lit("hello");
		printf("A string: \""STR_FMT"\"\n", STR_UNPACK(str));
		str_free(str);

		char** str_arr = array_new(char*, .is_ptr = 1, .defer_fn = str_free, .cmp_fn = str_cmp, .print_fn = str_print);
		char* strs[5] = { str_from_lit("hello"), str_from_lit("world"), str_from_lit("how"), str_from_lit("are"), str_from_lit("you") };
		array_append_mult_n(str_arr, 5, strs);
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
		int* set_res = hashset_union(set1, set2);
		hashset_print(set_res);
		hashset_free(set_res);
		printf("\nA n B =\t");
		set_res = hashset_intersec(set1, set2);
		hashset_print(set_res);
		hashset_free(set_res);
		printf("\nA - B =\t");
		set_res = hashset_diff(set1, set2);
		hashset_print(set_res);
		hashset_free(set_res);
		printf("\nB - A =\t");
		set_res = hashset_diff(set2, set1);
		hashset_print(set_res);
		hashset_free(set_res);
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
	
	// hashmap example
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

		printf("Removed: [ ");
		for (size_t i = 0; i < 10; ++i) {
			if (i) { printf(", "); }
			int r = rand() % 10;
			char* str = str_from_lit(strs[r]);
			printf("\""STR_FMT "\"",  STR_UNPACK(str));
			hashmap_remove(str_to_int, str, .defer = 1);
		}
		printf(" ]\n");

		printf("%zu: ", hashmap_size(str_to_int));
		hashmap_print(str_to_int);
		putchar('\n');

		hashmap_free(str_to_int);

		KV2* str_to_arr = hashmap_new(KV2, hash_str, str_eq,
			.defer_key_fn = str_free, .defer_value_fn = array_free,
			.is_key_ptr = 1, .is_value_ptr = 1,
			.print_key_fn = str_print, .print_value_fn = array_print
		);

		char* new_words[5] = { "a", "b", "c", "d", "e" };
		for (size_t i = 0; i < 5; ++i) {
			int* hashmap_arr = array_new(int, .print_fn = int_print);
			size_t r = rand() % 10;
			for (size_t j = 0; j < r; ++j) {
				array_append(hashmap_arr, rand() % 20);
			}
			hashmap_add_v(str_to_arr, str_from_lit(new_words[i]), hashmap_arr);
		}
		hashmap_print(str_to_arr);
		putchar('\n');
		hashmap_free(str_to_arr);
	}

	// binary heap example
	printf("\nBinaryHeap examples:\n"); {
		int* int_heap = binheap_new(int, int_compare, .print_fn = int_print);
		printf("Added: [ ");
		for (size_t i = 0; i < 20; ++i) {
			int r = rand() % 50 - 25;
			if (i) printf(", ");
			printf("%d", r);
			binheap_insert(int_heap, r);
		}
		printf(" ]\n");
		binheap_print(int_heap);
		putchar('\n');

		printf("Extracted: [ ");
		char started = 0;
		binheap_drain(extracted, int_heap) {
			if (started) { printf(", "); } else { started = 1; }
			printf("%d", *extracted);
		}
		printf(" ]\n");

		for (size_t i = 0; i < 10; ++i) {
			binheap_insert(int_heap, rand() % 20 - 10);
		}
		binheap_print(int_heap);
		putchar('\n');
		for (int i = -5; i < 5; ++i) {
			binheap_remove(int_heap, i);
		}
		binheap_print(int_heap);
		putchar('\n');

		binheap_free(int_heap);
	}

	// ring buffer example
	printf("\nRingBuffer examples:\n"); {
		int* int_ring = ring_new(int, .print_fn = int_print);
		for (size_t i = 0; i < 10; ++i) {
			ring_push(int_ring, rand() % 100);
		}
		ring_print(int_ring);
		putchar('\n');
		
		printf("Extracted: [ ");
		char started = 0;
		ring_drain(extracted, int_ring) {
			if (started) { printf(", "); } else { started = 1; }
			printf("%d", *extracted);
		}
		printf(" ]\n");

		ring_free(int_ring);
	}

	return 0;
}
