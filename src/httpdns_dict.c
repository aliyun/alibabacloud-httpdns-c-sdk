/* Hash Tables Implementation.
 *
 * This file implements in memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


#include "httpdns_dict.h"

/* Using httpdns_dict_enable_resize() / httpdns_dict_disable_resize() we make possible to
 * enable/disable resizing of the hash table as needed. This is very important
 * for Redis, as we use copy-on-write and don't want to move too much memory
 * around when there is a child performing saving operations.
 *
 * Note that even when dict_can_resize is set to 0, not all resizes are
 * prevented: an hash table is still allowed to grow if the ratio between
 * the number of elements and the buckets > dict_force_resize_ratio. */
static int dict_can_resize = 1;
static unsigned int dict_force_resize_ratio = 5;

/* -------------------------- private prototypes ---------------------------- */

static int _dictExpandIfNeeded(httpdns_dict_t *ht);
static unsigned long _dictNextPower(unsigned long size);
static int _dictKeyIndex(httpdns_dict_t *ht, const void *key);
static int _dictInit(httpdns_dict_t *ht, httpdns_dict_type_t *type, void *privDataPtr);

/* -------------------------- hash functions -------------------------------- */

/* Thomas Wang's 32 bit Mix Function */
#if 0
unsigned int dictIntHashFunction(unsigned int key)
{
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

/* Identity hash function for integer keys */
static unsigned int dictIdentityHashFunction(unsigned int key)
{
    return key;
}

#endif

/* Generic hash function (a popular one from Bernstein).
 * I tested a few and this was the best. */
unsigned int httpdns_dict_generate_hash_function(const unsigned char *buf, int len) {
    unsigned int hash = 5381;

    while (len--)
        hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
    return hash;
}

/* And a case insensitive version */
unsigned int httpdns_dict_generate_case_hash_function(const unsigned char *buf, int len) {
    unsigned int hash = 5381;

    while (len--)
        hash = ((hash << 5) + hash) + (tolower(*buf++)); /* hash * 33 + c */
    return hash;
}

/* ----------------------------- API implementation ------------------------- */

/* Reset an hashtable already initialized with ht_init().
 * NOTE: This function should only called by ht_destroy(). */
static void _dictReset(httpdns_dict_hash_table *ht)
{
    ht->table = NULL;
    ht->size = 0;
    ht->sizemask = 0;
    ht->used = 0;
}

/* Create a new hash table */
httpdns_dict_t *httpdns_dict_create(httpdns_dict_type_t *type,
        void *privDataPtr)
{
    httpdns_dict_t *d = malloc(sizeof(*d));

    _dictInit(d,type,privDataPtr);
    return d;
}

/* Initialize the hash table */
int _dictInit(httpdns_dict_t *d, httpdns_dict_type_t *type,
        void *privDataPtr)
{
    _dictReset(&d->ht[0]);
    _dictReset(&d->ht[1]);
    d->type = type;
    d->privdata = privDataPtr;
    d->rehashidx = -1;
    d->iterators = 0;
    return HTTPDNS_DICT_OK;
}

/* Resize the table to the minimal size that contains all the elements,
 * but with the invariant of a USER/BUCKETS ratio near to <= 1 */
int httpdns_dict_resize(httpdns_dict_t *d)
{
    int minimal;

    if (!dict_can_resize || httpdns_dict_list_rehashing(d)) return HTTPDNS_DICT_ERR;
    minimal = d->ht[0].used;
    if (minimal < HTTPDNS_DICT_HT_INITIAL_SIZE)
        minimal = HTTPDNS_DICT_HT_INITIAL_SIZE;
    return httpdns_dict_expand(d, minimal);
}

/* Expand or create the hashtable */
int httpdns_dict_expand(httpdns_dict_t *d, unsigned long size)
{
    httpdns_dict_hash_table n; /* the new hashtable */
    unsigned long realsize = _dictNextPower(size);

    /* the size is invalid if it is smaller than the number of
     * elements already inside the hashtable */
    if (httpdns_dict_list_rehashing(d) || d->ht[0].used > size)
        return HTTPDNS_DICT_ERR;

    /* Allocate the new hashtable and initialize all pointers to NULL */
    n.size = realsize;
    n.sizemask = realsize-1;
    n.table = calloc(realsize, sizeof(httpdns_dict_entry_t*));
    n.used = 0;

    /* Is this the first initialization? If so it's not really a rehashing
     * we just set the first hash table so that it can accept keys. */
    if (d->ht[0].table == NULL) {
        d->ht[0] = n;
        return HTTPDNS_DICT_OK;
    }

    /* Prepare a second hash table for incremental rehashing */
    d->ht[1] = n;
    d->rehashidx = 0;
    return HTTPDNS_DICT_OK;
}

/* Performs N steps of incremental rehashing. Returns 1 if there are still
 * keys to move from the old to the new hash table, otherwise 0 is returned.
 * Note that a rehashing step consists in moving a bucket (that may have more
 * thank one key as we use chaining) from the old to the new hash table. */
int httpdns_dict_rehash(httpdns_dict_t *d, int n) {
    if (!httpdns_dict_list_rehashing(d)) return 0;

    while(n--) {
        httpdns_dict_entry_t *de, *nextde;

        /* Check if we already rehashed the whole table... */
        if (d->ht[0].used == 0) {
            free(d->ht[0].table);
            d->ht[0] = d->ht[1];
            _dictReset(&d->ht[1]);
            d->rehashidx = -1;
            return 0;
        }

        /* Note that rehashidx can't overflow as we are sure there are more
         * elements because ht[0].used != 0 */
        while(d->ht[0].table[d->rehashidx] == NULL) d->rehashidx++;
        de = d->ht[0].table[d->rehashidx];
        /* Move all the keys in this bucket from the old to the new hash HT */
        while(de) {
            unsigned int h;

            nextde = de->next;
            /* Get the index in the new hash table */
            h = httpdns_dict_hash_key(d, de->key) & d->ht[1].sizemask;
            de->next = d->ht[1].table[h];
            d->ht[1].table[h] = de;
            d->ht[0].used--;
            d->ht[1].used++;
            de = nextde;
        }
        d->ht[0].table[d->rehashidx] = NULL;
        d->rehashidx++;
    }
    return 1;
}

static long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

/* Rehash for an amount of time between ms milliseconds and ms+1 milliseconds */
int httpdns_dict_rehash_milliseconds(httpdns_dict_t *d, int ms) {
    long long start = timeInMilliseconds();
    int rehashes = 0;

    while(httpdns_dict_rehash(d,100)) {
        rehashes += 100;
        if (timeInMilliseconds()-start > ms) break;
    }
    return rehashes;
}

/* This function performs just a step of rehashing, and only if there are
 * no safe iterators bound to our hash table. When we have iterators in the
 * middle of a rehashing we can't mess with the two hash tables otherwise
 * some element can be missed or duplicated.
 *
 * This function is called by common lookup or update operations in the
 * dictionary so that the hash table automatically migrates from H1 to H2
 * while it is actively used. */
static void _dictRehashStep(httpdns_dict_t *d) {
    if (d->iterators == 0) httpdns_dict_rehash(d,1);
}

/* Add an element to the target hash table */
int httpdns_dict_add(httpdns_dict_t *d, void *key, void *val)
{
    int index;
    httpdns_dict_entry_t *entry;
    httpdns_dict_hash_table *ht;

    if (httpdns_dict_list_rehashing(d)) _dictRehashStep(d);

    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if ((index = _dictKeyIndex(d, key)) == -1)
        return HTTPDNS_DICT_ERR;

    /* Allocates the memory and stores key */
    ht = httpdns_dict_list_rehashing(d) ? &d->ht[1] : &d->ht[0];
    entry = malloc(sizeof(*entry));
    entry->next = ht->table[index];
    ht->table[index] = entry;
    ht->used++;

    /* Set the hash entry fields. */
    httpdns_dict_set_hash_key(d, entry, key);
    httpdns_dict_set_hash_val(d, entry, val);
    return HTTPDNS_DICT_OK;
}

/* Add an element, discarding the old if the key already exists.
 * Return 1 if the key was added from scratch, 0 if there was already an
 * element with such key and httpdns_dict_replace() just performed a value update
 * operation. */
int httpdns_dict_replace(httpdns_dict_t *d, void *key, void *val)
{
    httpdns_dict_entry_t *entry, auxentry;

    /* Try to add the element. If the key
     * does not exists httpdns_dict_add will suceed. */
    if (httpdns_dict_add(d, key, val) == HTTPDNS_DICT_OK)
        return 1;
    /* It already exists, get the entry */
    entry = httpdns_dict_find(d, key);
    /* Free the old value and set the new one */
    /* Set the new value and free the old one. Note that it is important
     * to do that in this order, as the value may just be exactly the same
     * as the previous one. In this context, think to reference counting,
     * you want to increment (set), and then decrement (free), and not the
     * reverse. */
    auxentry = *entry;
    httpdns_dict_set_hash_val(d, entry, val);
    httpdns_dict_free_entry_val(d, &auxentry);
    return 0;
}

/* Search and remove an element */
static int dictGenericDelete(httpdns_dict_t *d, const void *key, int nofree)
{
    unsigned int h, idx;
    httpdns_dict_entry_t *he, *prevHe;
    int table;

    if (d->ht[0].size == 0) return HTTPDNS_DICT_ERR; /* d->ht[0].table is NULL */
    if (httpdns_dict_list_rehashing(d)) _dictRehashStep(d);
    h = httpdns_dict_hash_key(d, key);

    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        prevHe = NULL;
        while(he) {
            if (httpdns_dict_compare_hash_keys(d, key, he->key)) {
                /* Unlink the element from the list */
                if (prevHe)
                    prevHe->next = he->next;
                else
                    d->ht[table].table[idx] = he->next;
                if (!nofree) {
                    httpdns_dict_free_entry_key(d, he);
                    httpdns_dict_free_entry_val(d, he);
                }
                free(he);
                d->ht[table].used--;
                return HTTPDNS_DICT_OK;
            }
            prevHe = he;
            he = he->next;
        }
        if (!httpdns_dict_list_rehashing(d)) break;
    }
    return HTTPDNS_DICT_ERR; /* not found */
}

int httpdns_dict_delete(httpdns_dict_t *ht, const void *key) {
    return dictGenericDelete(ht,key,0);
}

int httpdns_dict_delete_no_free(httpdns_dict_t *ht, const void *key) {
    return dictGenericDelete(ht,key,1);
}

/* Destroy an entire dictionary */
static int _dictClear(httpdns_dict_t *d, httpdns_dict_hash_table *ht)
{
    unsigned long i;

    /* Free all the elements */
    for (i = 0; i < ht->size && ht->used > 0; i++) {
        httpdns_dict_entry_t *he, *nextHe;

        if ((he = ht->table[i]) == NULL) continue;
        while(he) {
            nextHe = he->next;
            httpdns_dict_free_entry_key(d, he);
            httpdns_dict_free_entry_val(d, he);
            free(he);
            ht->used--;
            he = nextHe;
        }
    }
    /* Free the table and the allocated cache structure */
    free(ht->table);
    /* Re-initialize the table */
    _dictReset(ht);
    return HTTPDNS_DICT_OK; /* never fails */
}

/* Clear & Release the hash table */
void httpdns_dict_release(httpdns_dict_t *d)
{
    _dictClear(d,&d->ht[0]);
    _dictClear(d,&d->ht[1]);
    free(d);
}

httpdns_dict_entry_t *httpdns_dict_find(httpdns_dict_t *d, const void *key)
{
    httpdns_dict_entry_t *he;
    unsigned int h, idx, table;

    if (d->ht[0].size == 0) return NULL; /* We don't have a table at all */
    if (httpdns_dict_list_rehashing(d)) _dictRehashStep(d);
    h = httpdns_dict_hash_key(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        while(he) {
            if (httpdns_dict_compare_hash_keys(d, key, he->key))
                return he;
            he = he->next;
        }
        if (!httpdns_dict_list_rehashing(d)) return NULL;
    }
    return NULL;
}

void *httpdns_dict_fetch_value(httpdns_dict_t *d, const void *key) {
    httpdns_dict_entry_t *he;

    he = httpdns_dict_find(d,key);
    return he ? httpdns_dict_get_entry_val(he) : NULL;
}

httpdns_dict_iterator_t *httpdns_dict_get_iterator(httpdns_dict_t *d)
{
    httpdns_dict_iterator_t *iter = malloc(sizeof(*iter));

    iter->d = d;
    iter->table = 0;
    iter->index = -1;
    iter->safe = 0;
    iter->entry = NULL;
    iter->nextEntry = NULL;
    return iter;
}

httpdns_dict_iterator_t *httpdns_dict_get_safe_iterator(httpdns_dict_t *d) {
    httpdns_dict_iterator_t *i = httpdns_dict_get_iterator(d);

    i->safe = 1;
    return i;
}

httpdns_dict_entry_t *httpdns_dict_next(httpdns_dict_iterator_t *iter)
{
    while (1) {
        if (iter->entry == NULL) {
            httpdns_dict_hash_table *ht = &iter->d->ht[iter->table];
            if (iter->safe && iter->index == -1 && iter->table == 0)
                iter->d->iterators++;
            iter->index++;
            if (iter->index >= (signed) ht->size) {
                if (httpdns_dict_list_rehashing(iter->d) && iter->table == 0) {
                    iter->table++;
                    iter->index = 0;
                    ht = &iter->d->ht[1];
                } else {
                    break;
                }
            }
            iter->entry = ht->table[iter->index];
        } else {
            iter->entry = iter->nextEntry;
        }
        if (iter->entry) {
            /* We need to save the 'next' here, the iterator user
             * may delete the entry we are returning. */
            iter->nextEntry = iter->entry->next;
            return iter->entry;
        }
    }
    return NULL;
}

void httpdns_dict_release_iterator(httpdns_dict_iterator_t *iter)
{
    if (iter->safe && !(iter->index == -1 && iter->table == 0))
        iter->d->iterators--;
    free(iter);
}

/* Return a random entry from the hash table. Useful to
 * implement randomized algorithms */
httpdns_dict_entry_t *httpdns_dict_get_random_key(httpdns_dict_t *d)
{
    httpdns_dict_entry_t *he, *orighe;
    unsigned int h;
    int listlen, listele;

    if (httpdns_dict_size(d) == 0) return NULL;
    if (httpdns_dict_list_rehashing(d)) _dictRehashStep(d);
    if (httpdns_dict_list_rehashing(d)) {
        do {
            h = random() % (d->ht[0].size+d->ht[1].size);
            he = (h >= d->ht[0].size) ? d->ht[1].table[h - d->ht[0].size] :
                                      d->ht[0].table[h];
        } while(he == NULL);
    } else {
        do {
            h = random() & d->ht[0].sizemask;
            he = d->ht[0].table[h];
        } while(he == NULL);
    }

    /* Now we found a non empty bucket, but it is a linked
     * list and we need to get a random element from the list.
     * The only sane way to do so is counting the elements and
     * select a random index. */
    listlen = 0;
    orighe = he;
    while(he) {
        he = he->next;
        listlen++;
    }
    listele = random() % listlen;
    he = orighe;
    while(listele--) he = he->next;
    return he;
}

/* ------------------------- private functions ------------------------------ */

/* Expand the hash table if needed */
static int _dictExpandIfNeeded(httpdns_dict_t *d)
{
    /* Incremental rehashing already in progress. Return. */
    if (httpdns_dict_list_rehashing(d)) return HTTPDNS_DICT_OK;

    /* If the hash table is empty expand it to the intial size. */
    if (d->ht[0].size == 0) return httpdns_dict_expand(d, HTTPDNS_DICT_HT_INITIAL_SIZE);

    /* If we reached the 1:1 ratio, and we are allowed to resize the hash
     * table (global setting) or we should avoid it but the ratio between
     * elements/buckets is over the "safe" threshold, we resize doubling
     * the number of buckets. */
    if (d->ht[0].used >= d->ht[0].size &&
        (dict_can_resize ||
         d->ht[0].used/d->ht[0].size > dict_force_resize_ratio))
    {
        return httpdns_dict_expand(d, ((d->ht[0].size > d->ht[0].used) ?
                                    d->ht[0].size : d->ht[0].used)*2);
    }
    return HTTPDNS_DICT_OK;
}

/* Our hash table capability is a power of two */
static unsigned long _dictNextPower(unsigned long size)
{
    unsigned long i = HTTPDNS_DICT_HT_INITIAL_SIZE;

    if (size >= LONG_MAX) return LONG_MAX;
    while(1) {
        if (i >= size)
            return i;
        i *= 2;
    }
}

/* Returns the index of a free slot that can be populated with
 * an hash entry for the given 'key'.
 * If the key already exists, -1 is returned.
 *
 * Note that if we are in the process of rehashing the hash table, the
 * index is always returned in the context of the second (new) hash table. */
static int _dictKeyIndex(httpdns_dict_t *d, const void *key)
{
    unsigned int h, idx, table;
    httpdns_dict_entry_t *he;

    /* Expand the hashtable if needed */
    if (_dictExpandIfNeeded(d) == HTTPDNS_DICT_ERR)
        return -1;
    /* Compute the key hash value */
    h = httpdns_dict_hash_key(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        /* Search if this slot does not already contain the given key */
        he = d->ht[table].table[idx];
        while(he) {
            if (httpdns_dict_compare_hash_keys(d, key, he->key))
                return -1;
            he = he->next;
        }
        if (!httpdns_dict_list_rehashing(d)) break;
    }
    return idx;
}

void httpdns_dict_empty(httpdns_dict_t *d) {
    _dictClear(d,&d->ht[0]);
    _dictClear(d,&d->ht[1]);
    d->rehashidx = -1;
    d->iterators = 0;
}

#define DICT_STATS_VECTLEN 50
static void _dictPrintStatsHt(httpdns_dict_hash_table *ht) {
    unsigned long i, slots = 0, chainlen, maxchainlen = 0;
    unsigned long totchainlen = 0;
    unsigned long clvector[DICT_STATS_VECTLEN];

    if (ht->used == 0) {
        printf("No stats available for empty dictionaries\n");
        return;
    }

    for (i = 0; i < DICT_STATS_VECTLEN; i++) clvector[i] = 0;
    for (i = 0; i < ht->size; i++) {
        httpdns_dict_entry_t *he;

        if (ht->table[i] == NULL) {
            clvector[0]++;
            continue;
        }
        slots++;
        /* For each hash entry on this slot... */
        chainlen = 0;
        he = ht->table[i];
        while(he) {
            chainlen++;
            he = he->next;
        }
        clvector[(chainlen < DICT_STATS_VECTLEN) ? chainlen : (DICT_STATS_VECTLEN-1)]++;
        if (chainlen > maxchainlen) maxchainlen = chainlen;
        totchainlen += chainlen;
    }
    printf("Hash table stats:\n");
    printf(" table size: %ld\n", ht->size);
    printf(" number of elements: %ld\n", ht->used);
    printf(" different slots: %ld\n", slots);
    printf(" max chain length: %ld\n", maxchainlen);
    printf(" avg chain length (counted): %.02f\n", (float)totchainlen/slots);
    printf(" avg chain length (computed): %.02f\n", (float)ht->used/slots);
    printf(" Chain length distribution:\n");
    for (i = 0; i < DICT_STATS_VECTLEN-1; i++) {
        if (clvector[i] == 0) continue;
        printf("   %s%ld: %ld (%.02f%%)\n",(i == DICT_STATS_VECTLEN-1)?">= ":"", i, clvector[i], ((float)clvector[i]/ht->size)*100);
    }
}

void httpdns_dict_print_stats(httpdns_dict_t *d) {
    _dictPrintStatsHt(&d->ht[0]);
    if (httpdns_dict_list_rehashing(d)) {
        printf("-- Rehashing into ht[1]:\n");
        _dictPrintStatsHt(&d->ht[1]);
    }
}

void httpdns_dict_enable_resize(void) {
    dict_can_resize = 1;
}

void httpdns_dict_disable_resize(void) {
    dict_can_resize = 0;
}


static unsigned int _dictStringCopyHTHashFunction(const void *key)
{
    return httpdns_dict_generate_hash_function(key, strlen(key));
}

static void *_dictStringDup(void *privdata, const void *key)
{
    int len = strlen(key);
    char *copy = malloc(len+1);
    HTTPDNS_DICT_NOTUSED(privdata);

    memcpy(copy, key, len);
    copy[len] = '\0';
    return copy;
}

static int _dictStringCopyHTKeyCompare(void *privdata, const void *key1,
        const void *key2)
{
    HTTPDNS_DICT_NOTUSED(privdata);

    return strcmp(key1, key2) == 0;
}

static void _dictStringDestructor(void *privdata, void *key)
{
    HTTPDNS_DICT_NOTUSED(privdata);

    free(key);
}

httpdns_dict_type_t httpdns_dict_type_heap_string_copy_key = {
    _dictStringCopyHTHashFunction, /* hash function */
    _dictStringDup,                /* key dup */
    NULL,                          /* val dup */
    _dictStringCopyHTKeyCompare,   /* key compare */
    _dictStringDestructor,         /* key destructor */
    NULL                           /* val destructor */
};

/* This is like StringCopy but does not auto-duplicate the key.
 * It's used for intepreter's shared strings. */
httpdns_dict_type_t httpdns_dict_type_heap_strings = {
    _dictStringCopyHTHashFunction, /* hash function */
    NULL,                          /* key dup */
    NULL,                          /* val dup */
    _dictStringCopyHTKeyCompare,   /* key compare */
    NULL,                          /* key destructor */
    NULL                           /* val destructor */
};

/* This is like StringCopy but also automatically handle dynamic
 * allocated C strings as values. */
httpdns_dict_type_t httpdns_dict_type_heap_string_copy_key_value = {
    _dictStringCopyHTHashFunction, /* hash function */
    _dictStringDup,                /* key dup */
    _dictStringDup,                /* val dup */
    _dictStringCopyHTKeyCompare,   /* key compare */
    _dictStringDestructor,         /* key destructor */
    _dictStringDestructor,         /* val destructor */
};
