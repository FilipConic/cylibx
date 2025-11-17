# CCollections

This library was inspired by Sean Barrett's [stb single header libraries](https://github.com/nothings/stb) and by this Youtube video by Dylan Falconer [Dynamic Arrays in C](https://www.youtube.com/watch?v=_KSKH8C9Gf0)

Things that this single header library provides are some of the usual generic *collection types*/*data structures* in other languages, but in C! Focus was on ergonomicity, security and speed. Not promising that it is fast but that it is fast while achiving the other two goals!

List of supported data structures:
 - array
 - bitmap
 - string
 - basic hash functions
 - hashset
 - hashmap
 - binary heap
 - ring buffer

## How to use the datastructures

A simple example using *arrays*:

```
    int* int_array = array_new(int);

    array_append_mult(int_array, 21, 9, 19, -23, 73, -41);
    for (size_t i = 0; i < array_length(int_array); ++i) { 
        printf("%zu: %d\n", i, int_array[i]);
    }

    array_free(int_array);
```

A more complex example using a *hashset*:

```
    int* int_set1 = hashset_new(int, hash_int, .print_fn);
    int* int_set2 = hashset_new(int, hash_int);

    hashset_add_mult(int_set1, 1, 2, 3, 4);
    hashset_add_mult(int_set2, 3, 4, 5);

    int* result = hashset_intersec(int_set1, int_set2);
    hashset_print(int_set1);
    printf(" n ");
    hashset_print(int_set2);
    printf(" = ");
    hashset_print(result);
    putchar('\n');

    hashset_free(int_set1);
    hashset_free(int_set2);
    hashset_free(result);
```

Output: `{ 3, 2, 4, 1 } n { 4, 5, 3 } = { 3, 4 }`

Even more complex example showing how to use the defer options and *hashmaps*:

Structure with fields key and value has to be created before use: `typedef struct { [void*] key; [void*] value; } KV;`

```
typedef struct {
    char* key;
    int* value;
} StringIntArrayKV;
    
    ...
    
    StringIntArrayKV* map = hashmap_new(StringIntArrayKV, hash_str, str_eq,
        .defer_key_fn = str_free, .defer_value_fn = array_free,
        .is_key_ptr = 1, .is_value_ptr = 1,
    );

    char* words[5] = { "a", "b", "c", "d", "e" };
    for (size_t i = 0; i < 5; ++i) {
        int* arr = array_new(int, .print_fn = int_print);
        array_append_mult(arr, 1, 2, 3);
        hashmap_add_v(map, str_from_lit(words[i]), arr);
    }

    hashmap_foreach(val_name, map) {
        printf(STR_FMT" : ", STR_UNPACK(val_name->key));
        array_print(val_name->value);
        putchar('\n');
    }

    hashmap_free(map);
```

Everything that is dynamically allocated in this examples is freed correctly! The *hashmap* is the most complex structure and it takes just a bit more setup than the other structures. Well if you have a repeating `hashmap_new` call you can always put it behind a macro! As the example show you can use a foreach loop. For structures like *hashmap* and *hashset* these are required but for others they are a nice thing to have.

## Important notices about using the library:
 - to use the library you need to add `#define CCOLLECTIONS_IMPLEMENTATION` above your `#include` macro for this library
 - the `.is_ptr` or `.is_key_ptr` or `.is_value_ptr` are flags that you need to set to *1* if you store a pointer to your data structure inside the data structure you are calling defer from
 - the `hash` and `equality` functions that need to be provided to `hashmap_new` are meant to be used for the *key* of the *hashmap*
 - the `cmp` function that needs to be provided to some the data structures expects < to return -1, > to return 1 and == to return 0
 - if you want to swap a *minheap* for a *maxheap* you need to change `cmp` function to return the opposite values, same for sorting an *array*

## TODOs
 - [ ] actually seperate code to be usable as a single header only library
 - [ ] better error reporting and error handling
 - [ ] ring buffer implementation
 - [ ] binary heap implementation
 - [ ] separate main into tests and examples 
 - [ ] add print functionallity behind a debug flag
 - [ ] add comments about function usage and possible quirks
 - [ ] make more tests
