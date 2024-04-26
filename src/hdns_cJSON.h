/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef HDNS_C_SDK_HDNS_CJSON_H
#define HDNS_C_SDK_HDNS_CJSON_H

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way CJSON_EXPORT_SYMBOLS does

*/

#define HDNS_CJSON_CDECL __cdecl
#define HDNS_CJSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define HDNS_CJSON_PUBLIC(type)   type HDNS_CJSON_STDCALL
#elif defined(CJSON_EXPORT_SYMBOLS)
#define HDNS_CJSON_PUBLIC(type)   __declspec(dllexport) type HDNS_CJSON_STDCALL
#elif defined(CJSON_IMPORT_SYMBOLS)
#define HDNS_CJSON_PUBLIC(type)   __declspec(dllimport) type HDNS_CJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define HDNS_CJSON_CDECL
#define HDNS_CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define HDNS_CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define HDNS_CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define HDNS_CJSON_VERSION_MAJOR 1
#define HDNS_CJSON_VERSION_MINOR 7
#define HDNS_CJSON_VERSION_PATCH 17

#include <stddef.h>

/* cJSON Types: */
#define hdns_cJSON_Invalid (0)
#define hdns_cJSON_False  (1 << 0)
#define hdns_cJSON_True   (1 << 1)
#define hdns_cJSON_NULL   (1 << 2)
#define hdns_cJSON_Number (1 << 3)
#define hdns_cJSON_String (1 << 4)
#define hdns_cJSON_Array  (1 << 5)
#define hdns_cJSON_Object (1 << 6)
#define hdns_cJSON_Raw    (1 << 7) /* raw json */

#define hdns_cJSON_IsReference 256
#define hdns_cJSON_StringIsConst 512

/* The cJSON structure: */
typedef struct hdns_cJSON_t
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct hdns_cJSON_t *next;
    struct hdns_cJSON_t *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct hdns_cJSON_t *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==hdns_cJSON_String  and type == hdns_cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} hdns_cJSON_t;

typedef struct hdns_cJSON_Hooks_t
{
      /* malloc/free are CDECL on Windows regardless of the default calling convention of the compiler, so ensure the hooks allow passing those functions directly. */
      void *(HDNS_CJSON_CDECL *malloc_fn)(size_t sz);
      void (HDNS_CJSON_CDECL *free_fn)(void *ptr);
} hdns_cJSON_Hooks_t;

typedef int hdns_cJSON_bool_t;

/* Limits how deeply nested arrays/objects can be before hdns_cJSON_t rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef HDNS_CJSON_NESTING_LIMIT
#define HDNS_CJSON_NESTING_LIMIT 1000
#endif

/* returns the version of hdns_cJSON_t as a string */
HDNS_CJSON_PUBLIC(const char*) hdns_cJSON_Version(void);

/* Supply malloc, realloc and free functions to hdns_cJSON_t */
HDNS_CJSON_PUBLIC(void) hdns_cJSON_InitHooks(hdns_cJSON_Hooks_t* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of hdns_cJSON_Parse (with hdns_cJSON_Delete) and hdns_cJSON_Print (with stdlib free, hdns_cJSON_Hooks.free_fn, or hdns_cJSON_free as appropriate). The exception is hdns_cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a hdns_cJSON_t object you can interrogate. */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_Parse(const char *value);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_ParseWithLength(const char *value, size_t buffer_length);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match hdns_cJSON_GetErrorPtr(). */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_ParseWithOpts(const char *value, const char **return_parse_end, hdns_cJSON_bool_t require_null_terminated);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, hdns_cJSON_bool_t require_null_terminated);

/* Render a hdns_cJSON_t entity to text for transfer/storage. */
HDNS_CJSON_PUBLIC(char *) hdns_cJSON_Print(const hdns_cJSON_t *item);
/* Render a hdns_cJSON_t entity to text for transfer/storage without any formatting. */
HDNS_CJSON_PUBLIC(char *) hdns_cJSON_PrintUnformatted(const hdns_cJSON_t *item);
/* Render a hdns_cJSON_t entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
HDNS_CJSON_PUBLIC(char *) hdns_cJSON_PrintBuffered(const hdns_cJSON_t *item, int prebuffer, hdns_cJSON_bool_t fmt);
/* Render a hdns_cJSON_t entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: hdns_cJSON_t is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_PrintPreallocated(hdns_cJSON_t *item, char *buffer, const int length, const hdns_cJSON_bool_t format);
/* Delete a hdns_cJSON_t entity and all subentities. */
HDNS_CJSON_PUBLIC(void) hdns_cJSON_Delete(hdns_cJSON_t *item);

/* Returns the number of items in an array (or object). */
HDNS_CJSON_PUBLIC(int) hdns_cJSON_GetArraySize(const hdns_cJSON_t *array);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_GetArrayItem(const hdns_cJSON_t *array, int index);
/* Get item "string" from object. Case insensitive. */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_GetObjectItem(const hdns_cJSON_t * const object, const char * const string);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_GetObjectItemCaseSensitive(const hdns_cJSON_t * const object, const char * const string);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_HasObjectItem(const hdns_cJSON_t *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when hdns_cJSON_Parse() returns 0. 0 when hdns_cJSON_Parse() succeeds. */
HDNS_CJSON_PUBLIC(const char *) hdns_cJSON_GetErrorPtr(void);

/* Check item type and return its value */
HDNS_CJSON_PUBLIC(char *) hdns_cJSON_GetStringValue(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(double) hdns_cJSON_GetNumberValue(const hdns_cJSON_t * const item);

/* These functions check the type of an item */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsInvalid(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsFalse(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsTrue(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsBool(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsNull(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsNumber(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsString(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsArray(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsObject(const hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_IsRaw(const hdns_cJSON_t * const item);

/* These calls create a hdns_cJSON_t item of the appropriate type. */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateNull(void);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateTrue(void);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateFalse(void);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateBool(hdns_cJSON_bool_t boolean);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateNumber(double num);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateString(const char *string);
/* raw json */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateRaw(const char *raw);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateArray(void);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by hdns_cJSON_Delete */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateStringReference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by hdns_cJSON_Delete */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateObjectReference(const hdns_cJSON_t *child);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateArrayReference(const hdns_cJSON_t *child);

/* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateIntArray(const int *numbers, int count);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateFloatArray(const float *numbers, int count);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateDoubleArray(const double *numbers, int count);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_CreateStringArray(const char *const *strings, int count);

/* Append item to the specified array/object. */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_AddItemToArray(hdns_cJSON_t *array, hdns_cJSON_t *item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_AddItemToObject(hdns_cJSON_t *object, const char *string, hdns_cJSON_t *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the hdns_cJSON_t object.
 * WARNING: When this function was used, make sure to always check that (item->type & hdns_cJSON_StringIsConst) is zero before
 * writing to `item->string` */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_AddItemToObjectCS(hdns_cJSON_t *object, const char *string, hdns_cJSON_t *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing hdns_cJSON_t to a new hdns_cJSON_t, but don't want to corrupt your existing hdns_cJSON_t. */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_AddItemReferenceToArray(hdns_cJSON_t *array, hdns_cJSON_t *item);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_AddItemReferenceToObject(hdns_cJSON_t *object, const char *string, hdns_cJSON_t *item);

/* Remove/Detach items from Arrays/Objects. */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_DetachItemViaPointer(hdns_cJSON_t *parent, hdns_cJSON_t * const item);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_DetachItemFromArray(hdns_cJSON_t *array, int which);
HDNS_CJSON_PUBLIC(void) hdns_cJSON_DeleteItemFromArray(hdns_cJSON_t *array, int which);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_DetachItemFromObject(hdns_cJSON_t *object, const char *string);
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_DetachItemFromObjectCaseSensitive(hdns_cJSON_t *object, const char *string);
HDNS_CJSON_PUBLIC(void) hdns_cJSON_DeleteItemFromObject(hdns_cJSON_t *object, const char *string);
HDNS_CJSON_PUBLIC(void) hdns_cJSON_DeleteItemFromObjectCaseSensitive(hdns_cJSON_t *object, const char *string);

/* Update array items. */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_InsertItemInArray(hdns_cJSON_t *array, int which, hdns_cJSON_t *newitem); /* Shifts pre-existing items to the right. */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_ReplaceItemViaPointer(hdns_cJSON_t * const parent, hdns_cJSON_t * const item, hdns_cJSON_t * replacement);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_ReplaceItemInArray(hdns_cJSON_t *array, int which, hdns_cJSON_t *newitem);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_ReplaceItemInObject(hdns_cJSON_t *object,const char *string,hdns_cJSON_t *newitem);
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_ReplaceItemInObjectCaseSensitive(hdns_cJSON_t *object,const char *string,hdns_cJSON_t *newitem);

/* Duplicate a hdns_cJSON_t item */
HDNS_CJSON_PUBLIC(hdns_cJSON_t *) hdns_cJSON_Duplicate(const hdns_cJSON_t *item, hdns_cJSON_bool_t recurse);
/* Duplicate will create a new, identical hdns_cJSON_t item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two hdns_cJSON_t items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
HDNS_CJSON_PUBLIC(hdns_cJSON_bool_t) hdns_cJSON_Compare(const hdns_cJSON_t * const a, const hdns_cJSON_t * const b, const hdns_cJSON_bool_t case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant, 
 * but should point to a readable and writable address area. */
HDNS_CJSON_PUBLIC(void) hdns_cJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddNullToObject(hdns_cJSON_t * const object, const char * const name);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddTrueToObject(hdns_cJSON_t * const object, const char * const name);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddFalseToObject(hdns_cJSON_t * const object, const char * const name);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddBoolToObject(hdns_cJSON_t * const object, const char * const name, const hdns_cJSON_bool_t boolean);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddNumberToObject(hdns_cJSON_t * const object, const char * const name, const double number);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddStringToObject(hdns_cJSON_t * const object, const char * const name, const char * const string);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddRawToObject(hdns_cJSON_t * const object, const char * const name, const char * const raw);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddObjectToObject(hdns_cJSON_t * const object, const char * const name);
HDNS_CJSON_PUBLIC(hdns_cJSON_t*) hdns_cJSON_AddArrayToObject(hdns_cJSON_t * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define hdns_cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the hdns_cJSON_SetNumberValue macro */
HDNS_CJSON_PUBLIC(double) hdns_cJSON_SetNumberHelper(hdns_cJSON_t *object, double number);
#define hdns_cJSON_SetNumberValue(object, number) ((object != NULL) ? hdns_cJSON_SetNumberHelper(object, (double)number) : (number))
/* Change the valuestring of a hdns_cJSON_String object, only takes effect when type of object is hdns_cJSON_String */
HDNS_CJSON_PUBLIC(char*) hdns_cJSON_SetValuestring(hdns_cJSON_t *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns hdns_cJSON_Invalid else it returns the new type*/
#define hdns_cJSON_SetBoolValue(object, boolValue) ( \
    (object != NULL && ((object)->type & (hdns_cJSON_False|hdns_cJSON_True))) ? \
    (object)->type=((object)->type &(~(hdns_cJSON_False|hdns_cJSON_True)))|((boolValue)?hdns_cJSON_True:hdns_cJSON_False) : \
    hdns_cJSON_Invalid\
)

/* Macro for iterating over an array or object */
#define hdns_cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with hdns_cJSON_InitHooks */
HDNS_CJSON_PUBLIC(void *) hdns_cJSON_malloc(size_t size);
HDNS_CJSON_PUBLIC(void) hdns_cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
