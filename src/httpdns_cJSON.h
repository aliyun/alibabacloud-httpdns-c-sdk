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

#ifndef HTTPDNS_C_SDK_HTTPDNS_CJSON_H
#define HTTPDNS_C_SDK_HTTPDNS_CJSON_H

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

#define HTTPDNS_CJSON_CDECL __cdecl
#define HTTPDNS_CJSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define HTTPDNS_CJSON_PUBLIC(type)   type HTTPDNS_CJSON_STDCALL
#elif defined(CJSON_EXPORT_SYMBOLS)
#define HTTPDNS_CJSON_PUBLIC(type)   __declspec(dllexport) type HTTPDNS_CJSON_STDCALL
#elif defined(CJSON_IMPORT_SYMBOLS)
#define HTTPDNS_CJSON_PUBLIC(type)   __declspec(dllimport) type HTTPDNS_CJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define HTTPDNS_CJSON_CDECL
#define HTTPDNS_CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define HTTPDNS_CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define HTTPDNS_CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define HTTPDNS_CJSON_VERSION_MAJOR 1
#define HTTPDNS_CJSON_VERSION_MINOR 7
#define HTTPDNS_CJSON_VERSION_PATCH 17

#include <stddef.h>

/* cJSON Types: */
#define httpdns_cJSON_Invalid (0)
#define httpdns_cJSON_False  (1 << 0)
#define httpdns_cJSON_True   (1 << 1)
#define httpdns_cJSON_NULL   (1 << 2)
#define httpdns_cJSON_Number (1 << 3)
#define httpdns_cJSON_String (1 << 4)
#define httpdns_cJSON_Array  (1 << 5)
#define httpdns_cJSON_Object (1 << 6)
#define httpdns_cJSON_Raw    (1 << 7) /* raw json */

#define httpdns_cJSON_IsReference 256
#define httpdns_cJSON_StringIsConst 512

/* The cJSON structure: */
typedef struct httpdns_cJSON_t
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct httpdns_cJSON_t *next;
    struct httpdns_cJSON_t *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct httpdns_cJSON_t *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==httpdns_cJSON_String  and type == httpdns_cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} httpdns_cJSON_t;

typedef struct httpdns_cJSON_Hooks_t
{
      /* malloc/free are CDECL on Windows regardless of the default calling convention of the compiler, so ensure the hooks allow passing those functions directly. */
      void *(HTTPDNS_CJSON_CDECL *malloc_fn)(size_t sz);
      void (HTTPDNS_CJSON_CDECL *free_fn)(void *ptr);
} httpdns_cJSON_Hooks_t;

typedef int httpdns_cJSON_bool_t;

/* Limits how deeply nested arrays/objects can be before httpdns_cJSON_t rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef HTTPDNS_CJSON_NESTING_LIMIT
#define HTTPDNS_CJSON_NESTING_LIMIT 1000
#endif

/* returns the version of httpdns_cJSON_t as a string */
HTTPDNS_CJSON_PUBLIC(const char*) httpdns_cJSON_Version(void);

/* Supply malloc, realloc and free functions to httpdns_cJSON_t */
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_InitHooks(httpdns_cJSON_Hooks_t* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of httpdns_cJSON_Parse (with httpdns_cJSON_Delete) and httpdns_cJSON_Print (with stdlib free, httpdns_cJSON_Hooks.free_fn, or httpdns_cJSON_free as appropriate). The exception is httpdns_cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a httpdns_cJSON_t object you can interrogate. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_Parse(const char *value);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_ParseWithLength(const char *value, size_t buffer_length);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match httpdns_cJSON_GetErrorPtr(). */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_ParseWithOpts(const char *value, const char **return_parse_end, httpdns_cJSON_bool_t require_null_terminated);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, httpdns_cJSON_bool_t require_null_terminated);

/* Render a httpdns_cJSON_t entity to text for transfer/storage. */
HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_Print(const httpdns_cJSON_t *item);
/* Render a httpdns_cJSON_t entity to text for transfer/storage without any formatting. */
HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_PrintUnformatted(const httpdns_cJSON_t *item);
/* Render a httpdns_cJSON_t entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_PrintBuffered(const httpdns_cJSON_t *item, int prebuffer, httpdns_cJSON_bool_t fmt);
/* Render a httpdns_cJSON_t entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: httpdns_cJSON_t is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_PrintPreallocated(httpdns_cJSON_t *item, char *buffer, const int length, const httpdns_cJSON_bool_t format);
/* Delete a httpdns_cJSON_t entity and all subentities. */
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_Delete(httpdns_cJSON_t *item);

/* Returns the number of items in an array (or object). */
HTTPDNS_CJSON_PUBLIC(int) httpdns_cJSON_GetArraySize(const httpdns_cJSON_t *array);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_GetArrayItem(const httpdns_cJSON_t *array, int index);
/* Get item "string" from object. Case insensitive. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_GetObjectItem(const httpdns_cJSON_t * const object, const char * const string);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_GetObjectItemCaseSensitive(const httpdns_cJSON_t * const object, const char * const string);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_HasObjectItem(const httpdns_cJSON_t *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when httpdns_cJSON_Parse() returns 0. 0 when httpdns_cJSON_Parse() succeeds. */
HTTPDNS_CJSON_PUBLIC(const char *) httpdns_cJSON_GetErrorPtr(void);

/* Check item type and return its value */
HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_GetStringValue(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(double) httpdns_cJSON_GetNumberValue(const httpdns_cJSON_t * const item);

/* These functions check the type of an item */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsInvalid(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsFalse(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsTrue(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsBool(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsNull(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsNumber(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsString(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsArray(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsObject(const httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsRaw(const httpdns_cJSON_t * const item);

/* These calls create a httpdns_cJSON_t item of the appropriate type. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateNull(void);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateTrue(void);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateFalse(void);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateBool(httpdns_cJSON_bool_t boolean);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateNumber(double num);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateString(const char *string);
/* raw json */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateRaw(const char *raw);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateArray(void);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by httpdns_cJSON_Delete */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateStringReference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by httpdns_cJSON_Delete */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateObjectReference(const httpdns_cJSON_t *child);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateArrayReference(const httpdns_cJSON_t *child);

/* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateIntArray(const int *numbers, int count);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateFloatArray(const float *numbers, int count);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateDoubleArray(const double *numbers, int count);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateStringArray(const char *const *strings, int count);

/* Append item to the specified array/object. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemToArray(httpdns_cJSON_t *array, httpdns_cJSON_t *item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemToObject(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the httpdns_cJSON_t object.
 * WARNING: When this function was used, make sure to always check that (item->type & httpdns_cJSON_StringIsConst) is zero before
 * writing to `item->string` */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemToObjectCS(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing httpdns_cJSON_t to a new httpdns_cJSON_t, but don't want to corrupt your existing httpdns_cJSON_t. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemReferenceToArray(httpdns_cJSON_t *array, httpdns_cJSON_t *item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemReferenceToObject(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *item);

/* Remove/Detach items from Arrays/Objects. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemViaPointer(httpdns_cJSON_t *parent, httpdns_cJSON_t * const item);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemFromArray(httpdns_cJSON_t *array, int which);
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_DeleteItemFromArray(httpdns_cJSON_t *array, int which);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemFromObject(httpdns_cJSON_t *object, const char *string);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemFromObjectCaseSensitive(httpdns_cJSON_t *object, const char *string);
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_DeleteItemFromObject(httpdns_cJSON_t *object, const char *string);
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_DeleteItemFromObjectCaseSensitive(httpdns_cJSON_t *object, const char *string);

/* Update array items. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_InsertItemInArray(httpdns_cJSON_t *array, int which, httpdns_cJSON_t *newitem); /* Shifts pre-existing items to the right. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemViaPointer(httpdns_cJSON_t * const parent, httpdns_cJSON_t * const item, httpdns_cJSON_t * replacement);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemInArray(httpdns_cJSON_t *array, int which, httpdns_cJSON_t *newitem);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemInObject(httpdns_cJSON_t *object,const char *string,httpdns_cJSON_t *newitem);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemInObjectCaseSensitive(httpdns_cJSON_t *object,const char *string,httpdns_cJSON_t *newitem);

/* Duplicate a httpdns_cJSON_t item */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_Duplicate(const httpdns_cJSON_t *item, httpdns_cJSON_bool_t recurse);
/* Duplicate will create a new, identical httpdns_cJSON_t item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two httpdns_cJSON_t items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_Compare(const httpdns_cJSON_t * const a, const httpdns_cJSON_t * const b, const httpdns_cJSON_bool_t case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant, 
 * but should point to a readable and writable address area. */
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddNullToObject(httpdns_cJSON_t * const object, const char * const name);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddTrueToObject(httpdns_cJSON_t * const object, const char * const name);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddFalseToObject(httpdns_cJSON_t * const object, const char * const name);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddBoolToObject(httpdns_cJSON_t * const object, const char * const name, const httpdns_cJSON_bool_t boolean);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddNumberToObject(httpdns_cJSON_t * const object, const char * const name, const double number);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddStringToObject(httpdns_cJSON_t * const object, const char * const name, const char * const string);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddRawToObject(httpdns_cJSON_t * const object, const char * const name, const char * const raw);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddObjectToObject(httpdns_cJSON_t * const object, const char * const name);
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddArrayToObject(httpdns_cJSON_t * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define httpdns_cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the httpdns_cJSON_SetNumberValue macro */
HTTPDNS_CJSON_PUBLIC(double) httpdns_cJSON_SetNumberHelper(httpdns_cJSON_t *object, double number);
#define httpdns_cJSON_SetNumberValue(object, number) ((object != NULL) ? httpdns_cJSON_SetNumberHelper(object, (double)number) : (number))
/* Change the valuestring of a httpdns_cJSON_String object, only takes effect when type of object is httpdns_cJSON_String */
HTTPDNS_CJSON_PUBLIC(char*) httpdns_cJSON_SetValuestring(httpdns_cJSON_t *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns httpdns_cJSON_Invalid else it returns the new type*/
#define httpdns_cJSON_SetBoolValue(object, boolValue) ( \
    (object != NULL && ((object)->type & (httpdns_cJSON_False|httpdns_cJSON_True))) ? \
    (object)->type=((object)->type &(~(httpdns_cJSON_False|httpdns_cJSON_True)))|((boolValue)?httpdns_cJSON_True:httpdns_cJSON_False) : \
    httpdns_cJSON_Invalid\
)

/* Macro for iterating over an array or object */
#define httpdns_cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with httpdns_cJSON_InitHooks */
HTTPDNS_CJSON_PUBLIC(void *) httpdns_cJSON_malloc(size_t size);
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
