/*
 * Hash Tables Implementation.
 *
 * 参考redis dict.h
 */
#ifdef __cplusplus
extern "C" {
#endif


#ifndef HTTPDNS_C_SDK_HTTPDNS_DICT_H
#define HTTPDNS_C_SDK_HTTPDNS_DICT_H

#define HTTPDNS_DICT_OK 0
#define HTTPDNS_DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define HTTPDNS_DICT_NOTUSED(V) ((void) V)

typedef struct httpdns_dict_entry_t {
    void *key;
    void *val;
    struct httpdns_dict_entry_t *next;
} httpdns_dict_entry_t;

typedef struct httpdns_dict_type_t {
    unsigned int (*hashFunction)(const void *key);
    void *(*keyDup)(void *privdata, const void *key);
    void *(*valDup)(void *privdata, const void *obj);
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} httpdns_dict_type_t;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
typedef struct httpdns_dict_hash_table {
    httpdns_dict_entry_t **table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
} httpdns_dict_hash_table;

typedef struct httpdns_dict_t {
    httpdns_dict_type_t *type;
    void *privdata;
    httpdns_dict_hash_table ht[2];
    int rehashidx; /* rehashing not in progress if rehashidx == -1 */
    int iterators; /* number of iterators currently running */
} httpdns_dict_t;

/* If safe is set to 1 this is a safe iteartor, that means, you can call
 * httpdns_dict_add, httpdns_dict_find, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only httpdns_dict_next()
 * should be called while iterating. */
typedef struct httpdns_dict_iterator_t {
    httpdns_dict_t *d;
    int table, index, safe;
    httpdns_dict_entry_t *entry, *nextEntry;
} httpdns_dict_iterator_t;

/* This is the initial size of every hash table */
#define HTTPDNS_DICT_HT_INITIAL_SIZE     4

/* ------------------------------- Macros ------------------------------------*/
#define httpdns_dict_free_entry_val(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->val)

#define httpdns_dict_set_hash_val(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        entry->val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        entry->val = (_val_); \
} while(0)

#define httpdns_dict_free_entry_key(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

#define httpdns_dict_set_hash_key(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        entry->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        entry->key = (_key_); \
} while(0)

#define httpdns_dict_compare_hash_keys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))

#define httpdns_dict_hash_key(d, key) (d)->type->hashFunction(key)

#define httpdns_dict_get_entry_key(he) ((he)->key)
#define httpdns_dict_get_entry_val(he) ((he)->val)
#define httpdns_dict_slots(d) ((d)->ht[0].size+(d)->ht[1].size)
#define httpdns_dict_size(d) ((d)->ht[0].used+(d)->ht[1].used)
#define httpdns_dict_list_rehashing(ht) ((ht)->rehashidx != -1)

/* API */
httpdns_dict_t *httpdns_dict_create(httpdns_dict_type_t *type, void *privDataPtr);
int httpdns_dict_expand(httpdns_dict_t *d, unsigned long size);
int httpdns_dict_add(httpdns_dict_t *d, void *key, void *val);
int httpdns_dict_replace(httpdns_dict_t *d, void *key, void *val);
int httpdns_dict_delete(httpdns_dict_t *d, const void *key);
int httpdns_dict_delete_no_free(httpdns_dict_t *d, const void *key);
void httpdns_dict_release(httpdns_dict_t *d);
httpdns_dict_entry_t * httpdns_dict_find(httpdns_dict_t *d, const void *key);
void *httpdns_dict_fetch_value(httpdns_dict_t *d, const void *key);
int httpdns_dict_resize(httpdns_dict_t *d);
httpdns_dict_iterator_t *httpdns_dict_get_iterator(httpdns_dict_t *d);
httpdns_dict_iterator_t *httpdns_dict_get_safe_iterator(httpdns_dict_t *d);
httpdns_dict_entry_t *httpdns_dict_next(httpdns_dict_iterator_t *iter);
void httpdns_dict_release_iterator(httpdns_dict_iterator_t *iter);
httpdns_dict_entry_t *httpdns_dict_get_random_key(httpdns_dict_t *d);
void httpdns_dict_print_stats(httpdns_dict_t *d);
unsigned int httpdns_dict_generate_hash_function(const unsigned char *buf, int len);
unsigned int httpdns_dict_generate_case_hash_function(const unsigned char *buf, int len);
void httpdns_dict_empty(httpdns_dict_t *d);
void httpdns_dict_enable_resize(void);
void httpdns_dict_disable_resize(void);
int httpdns_dict_rehash(httpdns_dict_t *d, int n);
int httpdns_dict_rehash_milliseconds(httpdns_dict_t *d, int ms);

/* Hash table types */
extern httpdns_dict_type_t httpdns_dict_type_heap_string_copy_key;
extern httpdns_dict_type_t httpdns_dict_type_heap_strings;
extern httpdns_dict_type_t httpdns_dict_type_heap_string_copy_key_value;

#ifdef __cplusplus
}
#endif

#endif /* HTTPDNS_C_SDK_HTTPDNS_DICT_H */
