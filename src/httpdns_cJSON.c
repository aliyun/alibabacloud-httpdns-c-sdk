/*
  Copyright (c) 2009-2017 Dave Gamble and httpdns_cJSON_t contributors

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

/* httpdns_cJSON_t */
/* JSON parser in C. */

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
#if defined(_MSC_VER)
#pragma warning (push)
/* disable warning about single line comments in system headers */
#pragma warning (disable : 4001)
#endif

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>

#ifdef ENABLE_LOCALES
#include <locale.h>
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif
#ifdef __GNUC__
#pragma GCC visibility pop
#endif

#include "httpdns_cJSON.h"

/* define our own boolean type */
#ifdef true
#undef true
#endif
#define true ((httpdns_cJSON_bool_t)1)

#ifdef false
#undef false
#endif
#define false ((httpdns_cJSON_bool_t)0)

/* define isnan and isinf for ANSI C, if in C99 or above, isnan and isinf has been defined in math.h */
#ifndef isinf
#define isinf(d) (isnan((d - d)) && !isnan(d))
#endif
#ifndef isnan
#define isnan(d) (d != d)
#endif

#ifndef NAN
#ifdef _WIN32
#define NAN sqrt(-1.0)
#else
#define NAN 0.0/0.0
#endif
#endif

typedef struct {
    const unsigned char *json;
    size_t position;
} httpdns_error_t;
static httpdns_error_t global_error = { NULL, 0 };

HTTPDNS_CJSON_PUBLIC(const char *) httpdns_cJSON_GetErrorPtr(void)
{
    return (const char*) (global_error.json + global_error.position);
}

HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_GetStringValue(const httpdns_cJSON_t * const item)
{
    if (!httpdns_cJSON_IsString(item))
    {
        return NULL;
    }

    return item->valuestring;
}

HTTPDNS_CJSON_PUBLIC(double) httpdns_cJSON_GetNumberValue(const httpdns_cJSON_t * const item)
{
    if (!httpdns_cJSON_IsNumber(item))
    {
        return (double) NAN;
    }

    return item->valuedouble;
}

/* This is a safeguard to prevent copy-pasters from using incompatible C and header files */
#if (HTTPDNS_CJSON_VERSION_MAJOR != 1) || (HTTPDNS_CJSON_VERSION_MINOR != 7) || (HTTPDNS_CJSON_VERSION_PATCH != 17)
    #httpdns_error_t httpdns_cJSON_t.h and httpdns_cJSON_t.c have different versions. Make sure that both have the same.
#endif

HTTPDNS_CJSON_PUBLIC(const char*) httpdns_cJSON_Version(void)
{
    static char version[15];
    sprintf(version, "%i.%i.%i", HTTPDNS_CJSON_VERSION_MAJOR, HTTPDNS_CJSON_VERSION_MINOR, HTTPDNS_CJSON_VERSION_PATCH);

    return version;
}

/* Case insensitive string comparison, doesn't consider two NULL pointers equal though */
static int case_insensitive_strcmp(const unsigned char *string1, const unsigned char *string2)
{
    if ((string1 == NULL) || (string2 == NULL))
    {
        return 1;
    }

    if (string1 == string2)
    {
        return 0;
    }

    for(; tolower(*string1) == tolower(*string2); (void)string1++, string2++)
    {
        if (*string1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*string1) - tolower(*string2);
}

typedef struct httpdns_internal_hooks_t
{
    void *(HTTPDNS_CJSON_CDECL *allocate)(size_t size);
    void (HTTPDNS_CJSON_CDECL *deallocate)(void *pointer);
    void *(HTTPDNS_CJSON_CDECL *reallocate)(void *pointer, size_t size);
} httpdns_internal_hooks_t;

#if defined(_MSC_VER)
/* work around MSVC httpdns_error_t C2322: '...' address of dllimport '...' is not static */
static void * HTTPDNS_CJSON_CDECL internal_malloc(size_t size)
{
    return malloc(size);
}
static void HTTPDNS_CJSON_CDECL internal_free(void *pointer)
{
    free(pointer);
}
static void * HTTPDNS_CJSON_CDECL internal_realloc(void *pointer, size_t size)
{
    return realloc(pointer, size);
}
#else
#define internal_malloc malloc
#define internal_free free
#define internal_realloc realloc
#endif

/* strlen of character literals resolved at compile time */
#define static_strlen(string_literal) (sizeof(string_literal) - sizeof(""))

static httpdns_internal_hooks_t global_hooks = { internal_malloc, internal_free, internal_realloc };

static unsigned char* httpdns_cJSON_strdup(const unsigned char* string, const httpdns_internal_hooks_t * const hooks)
{
    size_t length = 0;
    unsigned char *copy = NULL;

    if (string == NULL)
    {
        return NULL;
    }

    length = strlen((const char*)string) + sizeof("");
    copy = (unsigned char*)hooks->allocate(length);
    if (copy == NULL)
    {
        return NULL;
    }
    memcpy(copy, string, length);

    return copy;
}

HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_InitHooks(httpdns_cJSON_Hooks_t * hooks)
{
    if (hooks == NULL)
    {
        /* Reset hooks */
        global_hooks.allocate = malloc;
        global_hooks.deallocate = free;
        global_hooks.reallocate = realloc;
        return;
    }

    global_hooks.allocate = malloc;
    if (hooks->malloc_fn != NULL)
    {
        global_hooks.allocate = hooks->malloc_fn;
    }

    global_hooks.deallocate = free;
    if (hooks->free_fn != NULL)
    {
        global_hooks.deallocate = hooks->free_fn;
    }

    /* use realloc only if both free and malloc are used */
    global_hooks.reallocate = NULL;
    if ((global_hooks.allocate == malloc) && (global_hooks.deallocate == free))
    {
        global_hooks.reallocate = realloc;
    }
}

/* Internal constructor. */
static httpdns_cJSON_t *httpdns_cJSON_New_Item(const httpdns_internal_hooks_t * const hooks)
{
    httpdns_cJSON_t* node = (httpdns_cJSON_t*)hooks->allocate(sizeof(httpdns_cJSON_t));
    if (node)
    {
        memset(node, '\0', sizeof(httpdns_cJSON_t));
    }

    return node;
}

/* Delete a httpdns_cJSON_t structure. */
HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_Delete(httpdns_cJSON_t *item)
{
    httpdns_cJSON_t *next = NULL;
    while (item != NULL)
    {
        next = item->next;
        if (!(item->type & httpdns_cJSON_IsReference) && (item->child != NULL))
        {
            httpdns_cJSON_Delete(item->child);
        }
        if (!(item->type & httpdns_cJSON_IsReference) && (item->valuestring != NULL))
        {
            global_hooks.deallocate(item->valuestring);
        }
        if (!(item->type & httpdns_cJSON_StringIsConst) && (item->string != NULL))
        {
            global_hooks.deallocate(item->string);
        }
        global_hooks.deallocate(item);
        item = next;
    }
}

/* get the decimal point character of the current locale */
static unsigned char get_decimal_point(void)
{
#ifdef ENABLE_LOCALES
    struct lconv *lconv = localeconv();
    return (unsigned char) lconv->decimal_point[0];
#else
    return '.';
#endif
}

typedef struct
{
    const unsigned char *content;
    size_t length;
    size_t offset;
    size_t depth; /* How deeply nested (in arrays/objects) is the input at the current offset. */
    httpdns_internal_hooks_t hooks;
} parse_buffer;

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static httpdns_cJSON_bool_t parse_number(httpdns_cJSON_t * const item, parse_buffer * const input_buffer)
{
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char number_c_string[64];
    unsigned char decimal_point = get_decimal_point();
    size_t i = 0;

    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false;
    }

    /* copy the number into a temporary buffer and replace '.' with the decimal point
     * of the current locale (for strtod)
     * This also takes care of '\0' not necessarily being available for marking the end of the input */
    for (i = 0; (i < (sizeof(number_c_string) - 1)) && can_access_at_index(input_buffer, i); i++)
    {
        switch (buffer_at_offset(input_buffer)[i])
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
            case 'e':
            case 'E':
                number_c_string[i] = buffer_at_offset(input_buffer)[i];
                break;

            case '.':
                number_c_string[i] = decimal_point;
                break;

            default:
                goto loop_end;
        }
    }
loop_end:
    number_c_string[i] = '\0';

    number = strtod((const char*)number_c_string, (char**)&after_end);
    if (number_c_string == after_end)
    {
        return false; /* parse_error */
    }

    item->valuedouble = number;

    /* use saturation in case of overflow */
    if (number >= INT_MAX)
    {
        item->valueint = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        item->valueint = INT_MIN;
    }
    else
    {
        item->valueint = (int)number;
    }

    item->type = httpdns_cJSON_Number;

    input_buffer->offset += (size_t)(after_end - number_c_string);
    return true;
}

/* don't ask me, but the original httpdns_cJSON_SetNumberValue returns an integer or double */
HTTPDNS_CJSON_PUBLIC(double) httpdns_cJSON_SetNumberHelper(httpdns_cJSON_t *object, double number)
{
    if (number >= INT_MAX)
    {
        object->valueint = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        object->valueint = INT_MIN;
    }
    else
    {
        object->valueint = (int)number;
    }

    return object->valuedouble = number;
}

HTTPDNS_CJSON_PUBLIC(char*) httpdns_cJSON_SetValuestring(httpdns_cJSON_t *object, const char *valuestring)
{
    char *copy = NULL;
    /* if object's type is not httpdns_cJSON_String or is httpdns_cJSON_IsReference, it should not set valuestring */
    if ((object == NULL) || !(object->type & httpdns_cJSON_String) || (object->type & httpdns_cJSON_IsReference))
    {
        return NULL;
    }
    /* return NULL if the object is corrupted */
    if (object->valuestring == NULL)
    {
        return NULL;
    }
    if (strlen(valuestring) <= strlen(object->valuestring))
    {
        strcpy(object->valuestring, valuestring);
        return object->valuestring;
    }
    copy = (char*) httpdns_cJSON_strdup((const unsigned char*)valuestring, &global_hooks);
    if (copy == NULL)
    {
        return NULL;
    }
    if (object->valuestring != NULL)
    {
        httpdns_cJSON_free(object->valuestring);
    }
    object->valuestring = copy;

    return copy;
}

typedef struct
{
    unsigned char *buffer;
    size_t length;
    size_t offset;
    size_t depth; /* current nesting depth (for formatted printing) */
    httpdns_cJSON_bool_t noalloc;
    httpdns_cJSON_bool_t format; /* is this print a formatted print */
    httpdns_internal_hooks_t hooks;
} httpdns_printbuffer;

/* realloc httpdns_printbuffer if necessary to have at least "needed" bytes more */
static unsigned char* ensure(httpdns_printbuffer * const p, size_t needed)
{
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if ((p == NULL) || (p->buffer == NULL))
    {
        return NULL;
    }

    if ((p->length > 0) && (p->offset >= p->length))
    {
        /* make sure that offset is valid */
        return NULL;
    }

    if (needed > INT_MAX)
    {
        /* sizes bigger than INT_MAX are currently not supported */
        return NULL;
    }

    needed += p->offset + 1;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    if (p->noalloc) {
        return NULL;
    }

    /* calculate new buffer size */
    if (needed > (INT_MAX / 2))
    {
        /* overflow of int, use INT_MAX if possible */
        if (needed <= INT_MAX)
        {
            newsize = INT_MAX;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        newsize = needed * 2;
    }

    if (p->hooks.reallocate != NULL)
    {
        /* reallocate with realloc if available */
        newbuffer = (unsigned char*)p->hooks.reallocate(p->buffer, newsize);
        if (newbuffer == NULL)
        {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;

            return NULL;
        }
    }
    else
    {
        /* otherwise reallocate manually */
        newbuffer = (unsigned char*)p->hooks.allocate(newsize);
        if (!newbuffer)
        {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;

            return NULL;
        }

        memcpy(newbuffer, p->buffer, p->offset + 1);
        p->hooks.deallocate(p->buffer);
    }
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a httpdns_printbuffer and update the offset */
static void update_offset(httpdns_printbuffer * const buffer)
{
    const unsigned char *buffer_pointer = NULL;
    if ((buffer == NULL) || (buffer->buffer == NULL))
    {
        return;
    }
    buffer_pointer = buffer->buffer + buffer->offset;

    buffer->offset += strlen((const char*)buffer_pointer);
}

/* securely comparison of floating-point variables */
static httpdns_cJSON_bool_t compare_double(double a, double b)
{
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}

/* Render the number nicely from the given item into a string. */
static httpdns_cJSON_bool_t print_number(const httpdns_cJSON_t * const item, httpdns_printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    double d = item->valuedouble;
    int length = 0;
    size_t i = 0;
    unsigned char number_buffer[26] = {0}; /* temporary buffer to print the number into */
    unsigned char decimal_point = get_decimal_point();
    double test = 0.0;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* This checks for NaN and Infinity */
    if (isnan(d) || isinf(d))
    {
        length = sprintf((char*)number_buffer, "null");
    }
	else if(d == (double)item->valueint)
	{
		length = sprintf((char*)number_buffer, "%d", item->valueint);
	}
    else
    {
        /* Try 15 decimal places of precision to avoid nonsignificant nonzero digits */
        length = sprintf((char*)number_buffer, "%1.15g", d);

        /* Check whether the original double can be recovered */
        if ((sscanf((char*)number_buffer, "%lg", &test) != 1) || !compare_double((double)test, d))
        {
            /* If not, print with 17 decimal places of precision */
            length = sprintf((char*)number_buffer, "%1.17g", d);
        }
    }

    /* sprintf failed or buffer overrun occurred */
    if ((length < 0) || (length > (int)(sizeof(number_buffer) - 1)))
    {
        return false;
    }

    /* reserve appropriate space in the output */
    output_pointer = ensure(output_buffer, (size_t)length + sizeof(""));
    if (output_pointer == NULL)
    {
        return false;
    }

    /* copy the printed number to the output and replace locale
     * dependent decimal point with '.' */
    for (i = 0; i < ((size_t)length); i++)
    {
        if (number_buffer[i] == decimal_point)
        {
            output_pointer[i] = '.';
            continue;
        }

        output_pointer[i] = number_buffer[i];
    }
    output_pointer[i] = '\0';

    output_buffer->offset += (size_t)length;

    return true;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const unsigned char * const input)
{
    unsigned int h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++)
    {
        /* parse digit */
        if ((input[i] >= '0') && (input[i] <= '9'))
        {
            h += (unsigned int) input[i] - '0';
        }
        else if ((input[i] >= 'A') && (input[i] <= 'F'))
        {
            h += (unsigned int) 10 + input[i] - 'A';
        }
        else if ((input[i] >= 'a') && (input[i] <= 'f'))
        {
            h += (unsigned int) 10 + input[i] - 'a';
        }
        else /* invalid */
        {
            return 0;
        }

        if (i < 3)
        {
            /* shift left to make place for the next nibble */
            h = h << 4;
        }
    }

    return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static unsigned char utf16_literal_to_utf8(const unsigned char * const input_pointer, const unsigned char * const input_end, unsigned char **output_pointer)
{
    long unsigned int codepoint = 0;
    unsigned int first_code = 0;
    const unsigned char *first_sequence = input_pointer;
    unsigned char utf8_length = 0;
    unsigned char utf8_position = 0;
    unsigned char sequence_length = 0;
    unsigned char first_byte_mark = 0;

    if ((input_end - first_sequence) < 6)
    {
        /* input ends unexpectedly */
        goto fail;
    }

    /* get the first utf16 sequence */
    first_code = parse_hex4(first_sequence + 2);

    /* check that the code is valid */
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF)))
    {
        goto fail;
    }

    /* UTF16 surrogate pair */
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF))
    {
        const unsigned char *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12; /* \uXXXX\uXXXX */

        if ((input_end - second_sequence) < 6)
        {
            /* input ends unexpectedly */
            goto fail;
        }

        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u'))
        {
            /* missing second half of the surrogate pair */
            goto fail;
        }

        /* get the second utf16 sequence */
        second_code = parse_hex4(second_sequence + 2);
        /* check that the code is valid */
        if ((second_code < 0xDC00) || (second_code > 0xDFFF))
        {
            /* invalid second half of the surrogate pair */
            goto fail;
        }


        /* calculate the unicode codepoint from the surrogate pair */
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    }
    else
    {
        sequence_length = 6; /* \uXXXX */
        codepoint = first_code;
    }

    /* encode as UTF-8
     * takes at maximum 4 bytes to encode:
     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (codepoint < 0x80)
    {
        /* normal ascii, encoding 0xxxxxxx */
        utf8_length = 1;
    }
    else if (codepoint < 0x800)
    {
        /* two bytes, encoding 110xxxxx 10xxxxxx */
        utf8_length = 2;
        first_byte_mark = 0xC0; /* 11000000 */
    }
    else if (codepoint < 0x10000)
    {
        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
        utf8_length = 3;
        first_byte_mark = 0xE0; /* 11100000 */
    }
    else if (codepoint <= 0x10FFFF)
    {
        /* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_length = 4;
        first_byte_mark = 0xF0; /* 11110000 */
    }
    else
    {
        /* invalid unicode codepoint */
        goto fail;
    }

    /* encode as utf8 */
    for (utf8_position = (unsigned char)(utf8_length - 1); utf8_position > 0; utf8_position--)
    {
        /* 10xxxxxx */
        (*output_pointer)[utf8_position] = (unsigned char)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    /* encode first byte */
    if (utf8_length > 1)
    {
        (*output_pointer)[0] = (unsigned char)((codepoint | first_byte_mark) & 0xFF);
    }
    else
    {
        (*output_pointer)[0] = (unsigned char)(codepoint & 0x7F);
    }

    *output_pointer += utf8_length;

    return sequence_length;

fail:
    return 0;
}

/* Parse the input text into an unescaped cinput, and populate item. */
static httpdns_cJSON_bool_t parse_string(httpdns_cJSON_t * const item, parse_buffer * const input_buffer)
{
    const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
    unsigned char *output_pointer = NULL;
    unsigned char *output = NULL;

    /* not a string */
    if (buffer_at_offset(input_buffer)[0] != '\"')
    {
        goto fail;
    }

    {
        /* calculate approximate size of the output (overestimate) */
        size_t allocation_length = 0;
        size_t skipped_bytes = 0;
        while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && (*input_end != '\"'))
        {
            /* is escape sequence */
            if (input_end[0] == '\\')
            {
                if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length)
                {
                    /* prevent buffer overflow when last input character is a backslash */
                    goto fail;
                }
                skipped_bytes++;
                input_end++;
            }
            input_end++;
        }
        if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"'))
        {
            goto fail; /* string ended unexpectedly */
        }

        /* This is at most how much we need for the output */
        allocation_length = (size_t) (input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
        output = (unsigned char*)input_buffer->hooks.allocate(allocation_length + sizeof(""));
        if (output == NULL)
        {
            goto fail; /* allocation failure */
        }
    }

    output_pointer = output;
    /* loop through the string literal */
    while (input_pointer < input_end)
    {
        if (*input_pointer != '\\')
        {
            *output_pointer++ = *input_pointer++;
        }
        /* escape sequence */
        else
        {
            unsigned char sequence_length = 2;
            if ((input_end - input_pointer) < 1)
            {
                goto fail;
            }

            switch (input_pointer[1])
            {
                case 'b':
                    *output_pointer++ = '\b';
                    break;
                case 'f':
                    *output_pointer++ = '\f';
                    break;
                case 'n':
                    *output_pointer++ = '\n';
                    break;
                case 'r':
                    *output_pointer++ = '\r';
                    break;
                case 't':
                    *output_pointer++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *output_pointer++ = input_pointer[1];
                    break;

                /* UTF-16 literal */
                case 'u':
                    sequence_length = utf16_literal_to_utf8(input_pointer, input_end, &output_pointer);
                    if (sequence_length == 0)
                    {
                        /* failed to convert UTF16-literal to UTF-8 */
                        goto fail;
                    }
                    break;

                default:
                    goto fail;
            }
            input_pointer += sequence_length;
        }
    }

    /* zero terminate the output */
    *output_pointer = '\0';

    item->type = httpdns_cJSON_String;
    item->valuestring = (char*)output;

    input_buffer->offset = (size_t) (input_end - input_buffer->content);
    input_buffer->offset++;

    return true;

fail:
    if (output != NULL)
    {
        input_buffer->hooks.deallocate(output);
    }

    if (input_pointer != NULL)
    {
        input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    }

    return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static httpdns_cJSON_bool_t print_string_ptr(const unsigned char * const input, httpdns_printbuffer * const output_buffer)
{
    const unsigned char *input_pointer = NULL;
    unsigned char *output = NULL;
    unsigned char *output_pointer = NULL;
    size_t output_length = 0;
    /* numbers of additional characters needed for escaping */
    size_t escape_characters = 0;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* empty string */
    if (input == NULL)
    {
        output = ensure(output_buffer, sizeof("\"\""));
        if (output == NULL)
        {
            return false;
        }
        strcpy((char*)output, "\"\"");

        return true;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (input_pointer = input; *input_pointer; input_pointer++)
    {
        switch (*input_pointer)
        {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                /* one character escape sequence */
                escape_characters++;
                break;
            default:
                if (*input_pointer < 32)
                {
                    /* UTF-16 escape sequence uXXXX */
                    escape_characters += 5;
                }
                break;
        }
    }
    output_length = (size_t)(input_pointer - input) + escape_characters;

    output = ensure(output_buffer, output_length + sizeof("\"\""));
    if (output == NULL)
    {
        return false;
    }

    /* no characters have to be escaped */
    if (escape_characters == 0)
    {
        output[0] = '\"';
        memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';

        return true;
    }

    output[0] = '\"';
    output_pointer = output + 1;
    /* copy the string */
    for (input_pointer = input; *input_pointer != '\0'; (void)input_pointer++, output_pointer++)
    {
        if ((*input_pointer > 31) && (*input_pointer != '\"') && (*input_pointer != '\\'))
        {
            /* normal character, copy */
            *output_pointer = *input_pointer;
        }
        else
        {
            /* character needs to be escaped */
            *output_pointer++ = '\\';
            switch (*input_pointer)
            {
                case '\\':
                    *output_pointer = '\\';
                    break;
                case '\"':
                    *output_pointer = '\"';
                    break;
                case '\b':
                    *output_pointer = 'b';
                    break;
                case '\f':
                    *output_pointer = 'f';
                    break;
                case '\n':
                    *output_pointer = 'n';
                    break;
                case '\r':
                    *output_pointer = 'r';
                    break;
                case '\t':
                    *output_pointer = 't';
                    break;
                default:
                    /* escape and print as unicode codepoint */
                    sprintf((char*)output_pointer, "u%04x", *input_pointer);
                    output_pointer += 4;
                    break;
            }
        }
    }
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';

    return true;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static httpdns_cJSON_bool_t print_string(const httpdns_cJSON_t * const item, httpdns_printbuffer * const p)
{
    return print_string_ptr((unsigned char*)item->valuestring, p);
}

/* Predeclare these prototypes. */
static httpdns_cJSON_bool_t parse_value(httpdns_cJSON_t * const item, parse_buffer * const input_buffer);
static httpdns_cJSON_bool_t print_value(const httpdns_cJSON_t * const item, httpdns_printbuffer * const output_buffer);
static httpdns_cJSON_bool_t parse_array(httpdns_cJSON_t * const item, parse_buffer * const input_buffer);
static httpdns_cJSON_bool_t print_array(const httpdns_cJSON_t * const item, httpdns_printbuffer * const output_buffer);
static httpdns_cJSON_bool_t parse_object(httpdns_cJSON_t * const item, parse_buffer * const input_buffer);
static httpdns_cJSON_bool_t print_object(const httpdns_cJSON_t * const item, httpdns_printbuffer * const output_buffer);

/* Utility to jump whitespace and cr/lf */
static parse_buffer *buffer_skip_whitespace(parse_buffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL))
    {
        return NULL;
    }

    if (cannot_access_at_index(buffer, 0))
    {
        return buffer;
    }

    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32))
    {
       buffer->offset++;
    }

    if (buffer->offset == buffer->length)
    {
        buffer->offset--;
    }

    return buffer;
}

/* skip the UTF-8 BOM (byte order mark) if it is at the beginning of a buffer */
static parse_buffer *skip_utf8_bom(parse_buffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL) || (buffer->offset != 0))
    {
        return NULL;
    }

    if (can_access_at_index(buffer, 4) && (strncmp((const char*)buffer_at_offset(buffer), "\xEF\xBB\xBF", 3) == 0))
    {
        buffer->offset += 3;
    }

    return buffer;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_ParseWithOpts(const char *value, const char **return_parse_end, httpdns_cJSON_bool_t require_null_terminated)
{
    size_t buffer_length;

    if (NULL == value)
    {
        return NULL;
    }

    /* Adding null character size due to require_null_terminated. */
    buffer_length = strlen(value) + sizeof("");

    return httpdns_cJSON_ParseWithLengthOpts(value, buffer_length, return_parse_end, require_null_terminated);
}

/* Parse an object - create a new root, and populate. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, httpdns_cJSON_bool_t require_null_terminated)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    httpdns_cJSON_t *item = NULL;

    /* reset httpdns_error_t position */
    global_error.json = NULL;
    global_error.position = 0;

    if (value == NULL || 0 == buffer_length)
    {
        goto fail;
    }

    buffer.content = (const unsigned char*)value;
    buffer.length = buffer_length;
    buffer.offset = 0;
    buffer.hooks = global_hooks;

    item = httpdns_cJSON_New_Item(&global_hooks);
    if (item == NULL) /* memory fail */
    {
        goto fail;
    }

    if (!parse_value(item, buffer_skip_whitespace(skip_utf8_bom(&buffer))))
    {
        /* parse failure. ep is set. */
        goto fail;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated)
    {
        buffer_skip_whitespace(&buffer);
        if ((buffer.offset >= buffer.length) || buffer_at_offset(&buffer)[0] != '\0')
        {
            goto fail;
        }
    }
    if (return_parse_end)
    {
        *return_parse_end = (const char*)buffer_at_offset(&buffer);
    }

    return item;

fail:
    if (item != NULL)
    {
        httpdns_cJSON_Delete(item);
    }

    if (value != NULL)
    {
        httpdns_error_t local_error;
        local_error.json = (const unsigned char*)value;
        local_error.position = 0;

        if (buffer.offset < buffer.length)
        {
            local_error.position = buffer.offset;
        }
        else if (buffer.length > 0)
        {
            local_error.position = buffer.length - 1;
        }

        if (return_parse_end != NULL)
        {
            *return_parse_end = (const char*)local_error.json + local_error.position;
        }

        global_error = local_error;
    }

    return NULL;
}

/* Default options for httpdns_cJSON_Parse */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_Parse(const char *value)
{
    return httpdns_cJSON_ParseWithOpts(value, 0, 0);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_ParseWithLength(const char *value, size_t buffer_length)
{
    return httpdns_cJSON_ParseWithLengthOpts(value, buffer_length, 0, 0);
}

#define cjson_min(a, b) (((a) < (b)) ? (a) : (b))

static unsigned char *print(const httpdns_cJSON_t * const item, httpdns_cJSON_bool_t format, const httpdns_internal_hooks_t * const hooks)
{
    static const size_t default_buffer_size = 256;
    httpdns_printbuffer buffer[1];
    unsigned char *printed = NULL;

    memset(buffer, 0, sizeof(buffer));

    /* create buffer */
    buffer->buffer = (unsigned char*) hooks->allocate(default_buffer_size);
    buffer->length = default_buffer_size;
    buffer->format = format;
    buffer->hooks = *hooks;
    if (buffer->buffer == NULL)
    {
        goto fail;
    }

    /* print the value */
    if (!print_value(item, buffer))
    {
        goto fail;
    }
    update_offset(buffer);

    /* check if reallocate is available */
    if (hooks->reallocate != NULL)
    {
        printed = (unsigned char*) hooks->reallocate(buffer->buffer, buffer->offset + 1);
        if (printed == NULL) {
            goto fail;
        }
        buffer->buffer = NULL;
    }
    else /* otherwise copy the JSON over to a new buffer */
    {
        printed = (unsigned char*) hooks->allocate(buffer->offset + 1);
        if (printed == NULL)
        {
            goto fail;
        }
        memcpy(printed, buffer->buffer, cjson_min(buffer->length, buffer->offset + 1));
        printed[buffer->offset] = '\0'; /* just to be sure */

        /* free the buffer */
        hooks->deallocate(buffer->buffer);
    }

    return printed;

fail:
    if (buffer->buffer != NULL)
    {
        hooks->deallocate(buffer->buffer);
    }

    if (printed != NULL)
    {
        hooks->deallocate(printed);
    }

    return NULL;
}

/* Render a httpdns_cJSON_t item/entity/structure to text. */
HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_Print(const httpdns_cJSON_t *item)
{
    return (char*)print(item, true, &global_hooks);
}

HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_PrintUnformatted(const httpdns_cJSON_t *item)
{
    return (char*)print(item, false, &global_hooks);
}

HTTPDNS_CJSON_PUBLIC(char *) httpdns_cJSON_httpdns_printbuffered(const httpdns_cJSON_t *item, int prebuffer, httpdns_cJSON_bool_t fmt)
{
    httpdns_printbuffer p = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };

    if (prebuffer < 0)
    {
        return NULL;
    }

    p.buffer = (unsigned char*)global_hooks.allocate((size_t)prebuffer);
    if (!p.buffer)
    {
        return NULL;
    }

    p.length = (size_t)prebuffer;
    p.offset = 0;
    p.noalloc = false;
    p.format = fmt;
    p.hooks = global_hooks;

    if (!print_value(item, &p))
    {
        global_hooks.deallocate(p.buffer);
        return NULL;
    }

    return (char*)p.buffer;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_PrintPreallocated(httpdns_cJSON_t *item, char *buffer, const int length, const httpdns_cJSON_bool_t format)
{
    httpdns_printbuffer p = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };

    if ((length < 0) || (buffer == NULL))
    {
        return false;
    }

    p.buffer = (unsigned char*)buffer;
    p.length = (size_t)length;
    p.offset = 0;
    p.noalloc = true;
    p.format = format;
    p.hooks = global_hooks;

    return print_value(item, &p);
}

/* Parser core - when encountering text, process appropriately. */
static httpdns_cJSON_bool_t parse_value(httpdns_cJSON_t * const item, parse_buffer * const input_buffer)
{
    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false; /* no input */
    }

    /* parse the different types of values */
    /* null */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "null", 4) == 0))
    {
        item->type = httpdns_cJSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    /* false */
    if (can_read(input_buffer, 5) && (strncmp((const char*)buffer_at_offset(input_buffer), "false", 5) == 0))
    {
        item->type = httpdns_cJSON_False;
        input_buffer->offset += 5;
        return true;
    }
    /* true */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "true", 4) == 0))
    {
        item->type = httpdns_cJSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    /* string */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '\"'))
    {
        return parse_string(item, input_buffer);
    }
    /* number */
    if (can_access_at_index(input_buffer, 0) && ((buffer_at_offset(input_buffer)[0] == '-') || ((buffer_at_offset(input_buffer)[0] >= '0') && (buffer_at_offset(input_buffer)[0] <= '9'))))
    {
        return parse_number(item, input_buffer);
    }
    /* array */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '['))
    {
        return parse_array(item, input_buffer);
    }
    /* object */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '{'))
    {
        return parse_object(item, input_buffer);
    }

    return false;
}

/* Render a value to text. */
static httpdns_cJSON_bool_t print_value(const httpdns_cJSON_t * const item, httpdns_printbuffer * const output_buffer)
{
    unsigned char *output = NULL;

    if ((item == NULL) || (output_buffer == NULL))
    {
        return false;
    }

    switch ((item->type) & 0xFF)
    {
        case httpdns_cJSON_NULL:
            output = ensure(output_buffer, 5);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "null");
            return true;

        case httpdns_cJSON_False:
            output = ensure(output_buffer, 6);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "false");
            return true;

        case httpdns_cJSON_True:
            output = ensure(output_buffer, 5);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "true");
            return true;

        case httpdns_cJSON_Number:
            return print_number(item, output_buffer);

        case httpdns_cJSON_Raw:
        {
            size_t raw_length = 0;
            if (item->valuestring == NULL)
            {
                return false;
            }

            raw_length = strlen(item->valuestring) + sizeof("");
            output = ensure(output_buffer, raw_length);
            if (output == NULL)
            {
                return false;
            }
            memcpy(output, item->valuestring, raw_length);
            return true;
        }

        case httpdns_cJSON_String:
            return print_string(item, output_buffer);

        case httpdns_cJSON_Array:
            return print_array(item, output_buffer);

        case httpdns_cJSON_Object:
            return print_object(item, output_buffer);

        default:
            return false;
    }
}

/* Build an array from input text. */
static httpdns_cJSON_bool_t parse_array(httpdns_cJSON_t * const item, parse_buffer * const input_buffer)
{
    httpdns_cJSON_t *head = NULL; /* head of the linked list */
    httpdns_cJSON_t *current_item = NULL;

    if (input_buffer->depth >= HTTPDNS_CJSON_NESTING_LIMIT)
    {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (buffer_at_offset(input_buffer)[0] != '[')
    {
        /* not an array */
        goto fail;
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ']'))
    {
        /* empty array */
        goto success;
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        httpdns_cJSON_t *new_item = httpdns_cJSON_New_Item(&(input_buffer->hooks));
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse next value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer))
        {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']')
    {
        goto fail; /* expected end of array */
    }

success:
    input_buffer->depth--;

    if (head != NULL) {
        head->prev = current_item;
    }

    item->type = httpdns_cJSON_Array;
    item->child = head;

    input_buffer->offset++;

    return true;

fail:
    if (head != NULL)
    {
        httpdns_cJSON_Delete(head);
    }

    return false;
}

/* Render an array to text */
static httpdns_cJSON_bool_t print_array(const httpdns_cJSON_t * const item, httpdns_printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    httpdns_cJSON_t *current_element = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* Compose the output array. */
    /* opening square bracket */
    output_pointer = ensure(output_buffer, 1);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer = '[';
    output_buffer->offset++;
    output_buffer->depth++;

    while (current_element != NULL)
    {
        if (!print_value(current_element, output_buffer))
        {
            return false;
        }
        update_offset(output_buffer);
        if (current_element->next)
        {
            length = (size_t) (output_buffer->format ? 2 : 1);
            output_pointer = ensure(output_buffer, length + 1);
            if (output_pointer == NULL)
            {
                return false;
            }
            *output_pointer++ = ',';
            if(output_buffer->format)
            {
                *output_pointer++ = ' ';
            }
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }

    output_pointer = ensure(output_buffer, 2);
    if (output_pointer == NULL)
    {
        return false;
    }
    *output_pointer++ = ']';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Build an object from the text. */
static httpdns_cJSON_bool_t parse_object(httpdns_cJSON_t * const item, parse_buffer * const input_buffer)
{
    httpdns_cJSON_t *head = NULL; /* linked list head */
    httpdns_cJSON_t *current_item = NULL;

    if (input_buffer->depth >= HTTPDNS_CJSON_NESTING_LIMIT)
    {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '{'))
    {
        goto fail; /* not an object */
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '}'))
    {
        goto success; /* empty object */
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        httpdns_cJSON_t *new_item = httpdns_cJSON_New_Item(&(input_buffer->hooks));
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse the name of the child */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_string(current_item, input_buffer))
        {
            goto fail; /* failed to parse name */
        }
        buffer_skip_whitespace(input_buffer);

        /* swap valuestring and string, because we parsed the name */
        current_item->string = current_item->valuestring;
        current_item->valuestring = NULL;

        if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != ':'))
        {
            goto fail; /* invalid object */
        }

        /* parse the value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer))
        {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '}'))
    {
        goto fail; /* expected end of object */
    }

success:
    input_buffer->depth--;

    if (head != NULL) {
        head->prev = current_item;
    }

    item->type = httpdns_cJSON_Object;
    item->child = head;

    input_buffer->offset++;
    return true;

fail:
    if (head != NULL)
    {
        httpdns_cJSON_Delete(head);
    }

    return false;
}

/* Render an object to text. */
static httpdns_cJSON_bool_t print_object(const httpdns_cJSON_t * const item, httpdns_printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    httpdns_cJSON_t *current_item = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* Compose the output: */
    length = (size_t) (output_buffer->format ? 2 : 1); /* fmt: {\n */
    output_pointer = ensure(output_buffer, length + 1);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer++ = '{';
    output_buffer->depth++;
    if (output_buffer->format)
    {
        *output_pointer++ = '\n';
    }
    output_buffer->offset += length;

    while (current_item)
    {
        if (output_buffer->format)
        {
            size_t i;
            output_pointer = ensure(output_buffer, output_buffer->depth);
            if (output_pointer == NULL)
            {
                return false;
            }
            for (i = 0; i < output_buffer->depth; i++)
            {
                *output_pointer++ = '\t';
            }
            output_buffer->offset += output_buffer->depth;
        }

        /* print key */
        if (!print_string_ptr((unsigned char*)current_item->string, output_buffer))
        {
            return false;
        }
        update_offset(output_buffer);

        length = (size_t) (output_buffer->format ? 2 : 1);
        output_pointer = ensure(output_buffer, length);
        if (output_pointer == NULL)
        {
            return false;
        }
        *output_pointer++ = ':';
        if (output_buffer->format)
        {
            *output_pointer++ = '\t';
        }
        output_buffer->offset += length;

        /* print value */
        if (!print_value(current_item, output_buffer))
        {
            return false;
        }
        update_offset(output_buffer);

        /* print comma if not last */
        length = ((size_t)(output_buffer->format ? 1 : 0) + (size_t)(current_item->next ? 1 : 0));
        output_pointer = ensure(output_buffer, length + 1);
        if (output_pointer == NULL)
        {
            return false;
        }
        if (current_item->next)
        {
            *output_pointer++ = ',';
        }

        if (output_buffer->format)
        {
            *output_pointer++ = '\n';
        }
        *output_pointer = '\0';
        output_buffer->offset += length;

        current_item = current_item->next;
    }

    output_pointer = ensure(output_buffer, output_buffer->format ? (output_buffer->depth + 1) : 2);
    if (output_pointer == NULL)
    {
        return false;
    }
    if (output_buffer->format)
    {
        size_t i;
        for (i = 0; i < (output_buffer->depth - 1); i++)
        {
            *output_pointer++ = '\t';
        }
    }
    *output_pointer++ = '}';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Get Array size/item / object item. */
HTTPDNS_CJSON_PUBLIC(int) httpdns_cJSON_GetArraySize(const httpdns_cJSON_t *array)
{
    httpdns_cJSON_t *child = NULL;
    size_t size = 0;

    if (array == NULL)
    {
        return 0;
    }

    child = array->child;

    while(child != NULL)
    {
        size++;
        child = child->next;
    }

    /* FIXME: Can overflow here. Cannot be fixed without breaking the API */

    return (int)size;
}

static httpdns_cJSON_t* get_array_item(const httpdns_cJSON_t *array, size_t index)
{
    httpdns_cJSON_t *current_child = NULL;

    if (array == NULL)
    {
        return NULL;
    }

    current_child = array->child;
    while ((current_child != NULL) && (index > 0))
    {
        index--;
        current_child = current_child->next;
    }

    return current_child;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_GetArrayItem(const httpdns_cJSON_t *array, int index)
{
    if (index < 0)
    {
        return NULL;
    }

    return get_array_item(array, (size_t)index);
}

static httpdns_cJSON_t *get_object_item(const httpdns_cJSON_t * const object, const char * const name, const httpdns_cJSON_bool_t case_sensitive)
{
    httpdns_cJSON_t *current_element = NULL;

    if ((object == NULL) || (name == NULL))
    {
        return NULL;
    }

    current_element = object->child;
    if (case_sensitive)
    {
        while ((current_element != NULL) && (current_element->string != NULL) && (strcmp(name, current_element->string) != 0))
        {
            current_element = current_element->next;
        }
    }
    else
    {
        while ((current_element != NULL) && (case_insensitive_strcmp((const unsigned char*)name, (const unsigned char*)(current_element->string)) != 0))
        {
            current_element = current_element->next;
        }
    }

    if ((current_element == NULL) || (current_element->string == NULL)) {
        return NULL;
    }

    return current_element;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_GetObjectItem(const httpdns_cJSON_t * const object, const char * const string)
{
    return get_object_item(object, string, false);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_GetObjectItemCaseSensitive(const httpdns_cJSON_t * const object, const char * const string)
{
    return get_object_item(object, string, true);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_HasObjectItem(const httpdns_cJSON_t *object, const char *string)
{
    return httpdns_cJSON_GetObjectItem(object, string) ? 1 : 0;
}

/* Utility for array list handling. */
static void suffix_object(httpdns_cJSON_t *prev, httpdns_cJSON_t *item)
{
    prev->next = item;
    item->prev = prev;
}

/* Utility for handling references. */
static httpdns_cJSON_t *create_reference(const httpdns_cJSON_t *item, const httpdns_internal_hooks_t * const hooks)
{
    httpdns_cJSON_t *reference = NULL;
    if (item == NULL)
    {
        return NULL;
    }

    reference = httpdns_cJSON_New_Item(hooks);
    if (reference == NULL)
    {
        return NULL;
    }

    memcpy(reference, item, sizeof(httpdns_cJSON_t));
    reference->string = NULL;
    reference->type |= httpdns_cJSON_IsReference;
    reference->next = reference->prev = NULL;
    return reference;
}

static httpdns_cJSON_bool_t add_item_to_array(httpdns_cJSON_t *array, httpdns_cJSON_t *item)
{
    httpdns_cJSON_t *child = NULL;

    if ((item == NULL) || (array == NULL) || (array == item))
    {
        return false;
    }

    child = array->child;
    /*
     * To find the last item in array quickly, we use prev in array
     */
    if (child == NULL)
    {
        /* list is empty, start new one */
        array->child = item;
        item->prev = item;
        item->next = NULL;
    }
    else
    {
        /* append to the end */
        if (child->prev)
        {
            suffix_object(child->prev, item);
            array->child->prev = item;
        }
    }

    return true;
}

/* Add item to array/object. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemToArray(httpdns_cJSON_t *array, httpdns_cJSON_t *item)
{
    return add_item_to_array(array, item);
}

#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
/* helper function to cast away const */
static void* cast_away_const(const void* string)
{
    return (void*)string;
}
#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif


static httpdns_cJSON_bool_t add_item_to_object(httpdns_cJSON_t * const object, const char * const string, httpdns_cJSON_t * const item, const httpdns_internal_hooks_t * const hooks, const httpdns_cJSON_bool_t constant_key)
{
    char *new_key = NULL;
    int new_type = httpdns_cJSON_Invalid;

    if ((object == NULL) || (string == NULL) || (item == NULL) || (object == item))
    {
        return false;
    }

    if (constant_key)
    {
        new_key = (char*)cast_away_const(string);
        new_type = item->type | httpdns_cJSON_StringIsConst;
    }
    else
    {
        new_key = (char*)httpdns_cJSON_strdup((const unsigned char*)string, hooks);
        if (new_key == NULL)
        {
            return false;
        }

        new_type = item->type & ~httpdns_cJSON_StringIsConst;
    }

    if (!(item->type & httpdns_cJSON_StringIsConst) && (item->string != NULL))
    {
        hooks->deallocate(item->string);
    }

    item->string = new_key;
    item->type = new_type;

    return add_item_to_array(object, item);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemToObject(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *item)
{
    return add_item_to_object(object, string, item, &global_hooks, false);
}

/* Add an item to an object with constant string as key */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemToObjectCS(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *item)
{
    return add_item_to_object(object, string, item, &global_hooks, true);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemReferenceToArray(httpdns_cJSON_t *array, httpdns_cJSON_t *item)
{
    if (array == NULL)
    {
        return false;
    }

    return add_item_to_array(array, create_reference(item, &global_hooks));
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_AddItemReferenceToObject(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *item)
{
    if ((object == NULL) || (string == NULL))
    {
        return false;
    }

    return add_item_to_object(object, string, create_reference(item, &global_hooks), &global_hooks, false);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddNullToObject(httpdns_cJSON_t * const object, const char * const name)
{
    httpdns_cJSON_t *null = httpdns_cJSON_CreateNull();
    if (add_item_to_object(object, name, null, &global_hooks, false))
    {
        return null;
    }

    httpdns_cJSON_Delete(null);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddTrueToObject(httpdns_cJSON_t * const object, const char * const name)
{
    httpdns_cJSON_t *true_item = httpdns_cJSON_CreateTrue();
    if (add_item_to_object(object, name, true_item, &global_hooks, false))
    {
        return true_item;
    }

    httpdns_cJSON_Delete(true_item);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddFalseToObject(httpdns_cJSON_t * const object, const char * const name)
{
    httpdns_cJSON_t *false_item = httpdns_cJSON_CreateFalse();
    if (add_item_to_object(object, name, false_item, &global_hooks, false))
    {
        return false_item;
    }

    httpdns_cJSON_Delete(false_item);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddBoolToObject(httpdns_cJSON_t * const object, const char * const name, const httpdns_cJSON_bool_t boolean)
{
    httpdns_cJSON_t *bool_item = httpdns_cJSON_CreateBool(boolean);
    if (add_item_to_object(object, name, bool_item, &global_hooks, false))
    {
        return bool_item;
    }

    httpdns_cJSON_Delete(bool_item);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddNumberToObject(httpdns_cJSON_t * const object, const char * const name, const double number)
{
    httpdns_cJSON_t *number_item = httpdns_cJSON_CreateNumber(number);
    if (add_item_to_object(object, name, number_item, &global_hooks, false))
    {
        return number_item;
    }

    httpdns_cJSON_Delete(number_item);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddStringToObject(httpdns_cJSON_t * const object, const char * const name, const char * const string)
{
    httpdns_cJSON_t *string_item = httpdns_cJSON_CreateString(string);
    if (add_item_to_object(object, name, string_item, &global_hooks, false))
    {
        return string_item;
    }

    httpdns_cJSON_Delete(string_item);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddRawToObject(httpdns_cJSON_t * const object, const char * const name, const char * const raw)
{
    httpdns_cJSON_t *raw_item = httpdns_cJSON_CreateRaw(raw);
    if (add_item_to_object(object, name, raw_item, &global_hooks, false))
    {
        return raw_item;
    }

    httpdns_cJSON_Delete(raw_item);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddObjectToObject(httpdns_cJSON_t * const object, const char * const name)
{
    httpdns_cJSON_t *object_item = httpdns_cJSON_CreateObject();
    if (add_item_to_object(object, name, object_item, &global_hooks, false))
    {
        return object_item;
    }

    httpdns_cJSON_Delete(object_item);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t*) httpdns_cJSON_AddArrayToObject(httpdns_cJSON_t * const object, const char * const name)
{
    httpdns_cJSON_t *array = httpdns_cJSON_CreateArray();
    if (add_item_to_object(object, name, array, &global_hooks, false))
    {
        return array;
    }

    httpdns_cJSON_Delete(array);
    return NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemViaPointer(httpdns_cJSON_t *parent, httpdns_cJSON_t * const item)
{
    if ((parent == NULL) || (item == NULL))
    {
        return NULL;
    }

    if (item != parent->child)
    {
        /* not the first element */
        item->prev->next = item->next;
    }
    if (item->next != NULL)
    {
        /* not the last element */
        item->next->prev = item->prev;
    }

    if (item == parent->child)
    {
        /* first element */
        parent->child = item->next;
    }
    else if (item->next == NULL)
    {
        /* last element */
        parent->child->prev = item->prev;
    }

    /* make sure the detached item doesn't point anywhere anymore */
    item->prev = NULL;
    item->next = NULL;

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemFromArray(httpdns_cJSON_t *array, int which)
{
    if (which < 0)
    {
        return NULL;
    }

    return httpdns_cJSON_DetachItemViaPointer(array, get_array_item(array, (size_t)which));
}

HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_DeleteItemFromArray(httpdns_cJSON_t *array, int which)
{
    httpdns_cJSON_Delete(httpdns_cJSON_DetachItemFromArray(array, which));
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemFromObject(httpdns_cJSON_t *object, const char *string)
{
    httpdns_cJSON_t *to_detach = httpdns_cJSON_GetObjectItem(object, string);

    return httpdns_cJSON_DetachItemViaPointer(object, to_detach);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_DetachItemFromObjectCaseSensitive(httpdns_cJSON_t *object, const char *string)
{
    httpdns_cJSON_t *to_detach = httpdns_cJSON_GetObjectItemCaseSensitive(object, string);

    return httpdns_cJSON_DetachItemViaPointer(object, to_detach);
}

HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_DeleteItemFromObject(httpdns_cJSON_t *object, const char *string)
{
    httpdns_cJSON_Delete(httpdns_cJSON_DetachItemFromObject(object, string));
}

HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_DeleteItemFromObjectCaseSensitive(httpdns_cJSON_t *object, const char *string)
{
    httpdns_cJSON_Delete(httpdns_cJSON_DetachItemFromObjectCaseSensitive(object, string));
}

/* Replace array/object items with new ones. */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_InsertItemInArray(httpdns_cJSON_t *array, int which, httpdns_cJSON_t *newitem)
{
    httpdns_cJSON_t *after_inserted = NULL;

    if (which < 0 || newitem == NULL)
    {
        return false;
    }

    after_inserted = get_array_item(array, (size_t)which);
    if (after_inserted == NULL)
    {
        return add_item_to_array(array, newitem);
    }

    if (after_inserted != array->child && after_inserted->prev == NULL) {
        /* return false if after_inserted is a corrupted array item */
        return false;
    }

    newitem->next = after_inserted;
    newitem->prev = after_inserted->prev;
    after_inserted->prev = newitem;
    if (after_inserted == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
    return true;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemViaPointer(httpdns_cJSON_t * const parent, httpdns_cJSON_t * const item, httpdns_cJSON_t * replacement)
{
    if ((parent == NULL) || (parent->child == NULL) || (replacement == NULL) || (item == NULL))
    {
        return false;
    }

    if (replacement == item)
    {
        return true;
    }

    replacement->next = item->next;
    replacement->prev = item->prev;

    if (replacement->next != NULL)
    {
        replacement->next->prev = replacement;
    }
    if (parent->child == item)
    {
        if (parent->child->prev == parent->child)
        {
            replacement->prev = replacement;
        }
        parent->child = replacement;
    }
    else
    {   /*
         * To find the last item in array quickly, we use prev in array.
         * We can't modify the last item's next pointer where this item was the parent's child
         */
        if (replacement->prev != NULL)
        {
            replacement->prev->next = replacement;
        }
        if (replacement->next == NULL)
        {
            parent->child->prev = replacement;
        }
    }

    item->next = NULL;
    item->prev = NULL;
    httpdns_cJSON_Delete(item);

    return true;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemInArray(httpdns_cJSON_t *array, int which, httpdns_cJSON_t *newitem)
{
    if (which < 0)
    {
        return false;
    }

    return httpdns_cJSON_ReplaceItemViaPointer(array, get_array_item(array, (size_t)which), newitem);
}

static httpdns_cJSON_bool_t replace_item_in_object(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *replacement, httpdns_cJSON_bool_t case_sensitive)
{
    if ((replacement == NULL) || (string == NULL))
    {
        return false;
    }

    /* replace the name in the replacement */
    if (!(replacement->type & httpdns_cJSON_StringIsConst) && (replacement->string != NULL))
    {
        httpdns_cJSON_free(replacement->string);
    }
    replacement->string = (char*)httpdns_cJSON_strdup((const unsigned char*)string, &global_hooks);
    if (replacement->string == NULL)
    {
        return false;
    }

    replacement->type &= ~httpdns_cJSON_StringIsConst;

    return httpdns_cJSON_ReplaceItemViaPointer(object, get_object_item(object, string, case_sensitive), replacement);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemInObject(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *newitem)
{
    return replace_item_in_object(object, string, newitem, false);
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_ReplaceItemInObjectCaseSensitive(httpdns_cJSON_t *object, const char *string, httpdns_cJSON_t *newitem)
{
    return replace_item_in_object(object, string, newitem, true);
}

/* Create basic types: */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateNull(void)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = httpdns_cJSON_NULL;
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateTrue(void)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = httpdns_cJSON_True;
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateFalse(void)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = httpdns_cJSON_False;
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateBool(httpdns_cJSON_bool_t boolean)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = boolean ? httpdns_cJSON_True : httpdns_cJSON_False;
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateNumber(double num)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = httpdns_cJSON_Number;
        item->valuedouble = num;

        /* use saturation in case of overflow */
        if (num >= INT_MAX)
        {
            item->valueint = INT_MAX;
        }
        else if (num <= (double)INT_MIN)
        {
            item->valueint = INT_MIN;
        }
        else
        {
            item->valueint = (int)num;
        }
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateString(const char *string)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = httpdns_cJSON_String;
        item->valuestring = (char*)httpdns_cJSON_strdup((const unsigned char*)string, &global_hooks);
        if(!item->valuestring)
        {
            httpdns_cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateStringReference(const char *string)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if (item != NULL)
    {
        item->type = httpdns_cJSON_String | httpdns_cJSON_IsReference;
        item->valuestring = (char*)cast_away_const(string);
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateObjectReference(const httpdns_cJSON_t *child)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if (item != NULL) {
        item->type = httpdns_cJSON_Object | httpdns_cJSON_IsReference;
        item->child = (httpdns_cJSON_t*)cast_away_const(child);
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateArrayReference(const httpdns_cJSON_t *child) {
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if (item != NULL) {
        item->type = httpdns_cJSON_Array | httpdns_cJSON_IsReference;
        item->child = (httpdns_cJSON_t*)cast_away_const(child);
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateRaw(const char *raw)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = httpdns_cJSON_Raw;
        item->valuestring = (char*)httpdns_cJSON_strdup((const unsigned char*)raw, &global_hooks);
        if(!item->valuestring)
        {
            httpdns_cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateArray(void)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type=httpdns_cJSON_Array;
    }

    return item;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateObject(void)
{
    httpdns_cJSON_t *item = httpdns_cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = httpdns_cJSON_Object;
    }

    return item;
}

/* Create Arrays: */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateIntArray(const int *numbers, int count)
{
    size_t i = 0;
    httpdns_cJSON_t *n = NULL;
    httpdns_cJSON_t *p = NULL;
    httpdns_cJSON_t *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = httpdns_cJSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = httpdns_cJSON_CreateNumber(numbers[i]);
        if (!n)
        {
            httpdns_cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateFloatArray(const float *numbers, int count)
{
    size_t i = 0;
    httpdns_cJSON_t *n = NULL;
    httpdns_cJSON_t *p = NULL;
    httpdns_cJSON_t *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = httpdns_cJSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = httpdns_cJSON_CreateNumber((double)numbers[i]);
        if(!n)
        {
            httpdns_cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateDoubleArray(const double *numbers, int count)
{
    size_t i = 0;
    httpdns_cJSON_t *n = NULL;
    httpdns_cJSON_t *p = NULL;
    httpdns_cJSON_t *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = httpdns_cJSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = httpdns_cJSON_CreateNumber(numbers[i]);
        if(!n)
        {
            httpdns_cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_CreateStringArray(const char *const *strings, int count)
{
    size_t i = 0;
    httpdns_cJSON_t *n = NULL;
    httpdns_cJSON_t *p = NULL;
    httpdns_cJSON_t *a = NULL;

    if ((count < 0) || (strings == NULL))
    {
        return NULL;
    }

    a = httpdns_cJSON_CreateArray();

    for (i = 0; a && (i < (size_t)count); i++)
    {
        n = httpdns_cJSON_CreateString(strings[i]);
        if(!n)
        {
            httpdns_cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p,n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

/* Duplication */
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_t *) httpdns_cJSON_Duplicate(const httpdns_cJSON_t *item, httpdns_cJSON_bool_t recurse)
{
    httpdns_cJSON_t *newitem = NULL;
    httpdns_cJSON_t *child = NULL;
    httpdns_cJSON_t *next = NULL;
    httpdns_cJSON_t *newchild = NULL;

    /* Bail on bad ptr */
    if (!item)
    {
        goto fail;
    }
    /* Create new item */
    newitem = httpdns_cJSON_New_Item(&global_hooks);
    if (!newitem)
    {
        goto fail;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~httpdns_cJSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring)
    {
        newitem->valuestring = (char*)httpdns_cJSON_strdup((unsigned char*)item->valuestring, &global_hooks);
        if (!newitem->valuestring)
        {
            goto fail;
        }
    }
    if (item->string)
    {
        newitem->string = (item->type&httpdns_cJSON_StringIsConst) ? item->string : (char*)httpdns_cJSON_strdup((unsigned char*)item->string, &global_hooks);
        if (!newitem->string)
        {
            goto fail;
        }
    }
    /* If non-recursive, then we're done! */
    if (!recurse)
    {
        return newitem;
    }
    /* Walk the ->next chain for the child. */
    child = item->child;
    while (child != NULL)
    {
        newchild = httpdns_cJSON_Duplicate(child, true); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild)
        {
            goto fail;
        }
        if (next != NULL)
        {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            next->next = newchild;
            newchild->prev = next;
            next = newchild;
        }
        else
        {
            /* Set newitem->child and move to it */
            newitem->child = newchild;
            next = newchild;
        }
        child = child->next;
    }
    if (newitem && newitem->child)
    {
        newitem->child->prev = newchild;
    }

    return newitem;

fail:
    if (newitem != NULL)
    {
        httpdns_cJSON_Delete(newitem);
    }

    return NULL;
}

static void skip_oneline_comment(char **input)
{
    *input += static_strlen("//");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if ((*input)[0] == '\n') {
            *input += static_strlen("\n");
            return;
        }
    }
}

static void skip_multiline_comment(char **input)
{
    *input += static_strlen("/*");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if (((*input)[0] == '*') && ((*input)[1] == '/'))
        {
            *input += static_strlen("*/");
            return;
        }
    }
}

static void minify_string(char **input, char **output) {
    (*output)[0] = (*input)[0];
    *input += static_strlen("\"");
    *output += static_strlen("\"");


    for (; (*input)[0] != '\0'; (void)++(*input), ++(*output)) {
        (*output)[0] = (*input)[0];

        if ((*input)[0] == '\"') {
            (*output)[0] = '\"';
            *input += static_strlen("\"");
            *output += static_strlen("\"");
            return;
        } else if (((*input)[0] == '\\') && ((*input)[1] == '\"')) {
            (*output)[1] = (*input)[1];
            *input += static_strlen("\"");
            *output += static_strlen("\"");
        }
    }
}

HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_Minify(char *json)
{
    char *into = json;

    if (json == NULL)
    {
        return;
    }

    while (json[0] != '\0')
    {
        switch (json[0])
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                json++;
                break;

            case '/':
                if (json[1] == '/')
                {
                    skip_oneline_comment(&json);
                }
                else if (json[1] == '*')
                {
                    skip_multiline_comment(&json);
                } else {
                    json++;
                }
                break;

            case '\"':
                minify_string(&json, (char**)&into);
                break;

            default:
                into[0] = json[0];
                json++;
                into++;
        }
    }

    /* and null-terminate. */
    *into = '\0';
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsInvalid(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_Invalid;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsFalse(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_False;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsTrue(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xff) == httpdns_cJSON_True;
}


HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsBool(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & (httpdns_cJSON_True | httpdns_cJSON_False)) != 0;
}
HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsNull(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_NULL;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsNumber(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_Number;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsString(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_String;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsArray(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_Array;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsObject(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_Object;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_IsRaw(const httpdns_cJSON_t * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == httpdns_cJSON_Raw;
}

HTTPDNS_CJSON_PUBLIC(httpdns_cJSON_bool_t) httpdns_cJSON_Compare(const httpdns_cJSON_t * const a, const httpdns_cJSON_t * const b, const httpdns_cJSON_bool_t case_sensitive)
{
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF)))
    {
        return false;
    }

    /* check if type is valid */
    switch (a->type & 0xFF)
    {
        case httpdns_cJSON_False:
        case httpdns_cJSON_True:
        case httpdns_cJSON_NULL:
        case httpdns_cJSON_Number:
        case httpdns_cJSON_String:
        case httpdns_cJSON_Raw:
        case httpdns_cJSON_Array:
        case httpdns_cJSON_Object:
            break;

        default:
            return false;
    }

    /* identical objects are equal */
    if (a == b)
    {
        return true;
    }

    switch (a->type & 0xFF)
    {
        /* in these cases and equal type is enough */
        case httpdns_cJSON_False:
        case httpdns_cJSON_True:
        case httpdns_cJSON_NULL:
            return true;

        case httpdns_cJSON_Number:
            if (compare_double(a->valuedouble, b->valuedouble))
            {
                return true;
            }
            return false;

        case httpdns_cJSON_String:
        case httpdns_cJSON_Raw:
            if ((a->valuestring == NULL) || (b->valuestring == NULL))
            {
                return false;
            }
            if (strcmp(a->valuestring, b->valuestring) == 0)
            {
                return true;
            }

            return false;

        case httpdns_cJSON_Array:
        {
            httpdns_cJSON_t *a_element = a->child;
            httpdns_cJSON_t *b_element = b->child;

            for (; (a_element != NULL) && (b_element != NULL);)
            {
                if (!httpdns_cJSON_Compare(a_element, b_element, case_sensitive))
                {
                    return false;
                }

                a_element = a_element->next;
                b_element = b_element->next;
            }

            /* one of the arrays is longer than the other */
            if (a_element != b_element) {
                return false;
            }

            return true;
        }

        case httpdns_cJSON_Object:
        {
            httpdns_cJSON_t *a_element = NULL;
            httpdns_cJSON_t *b_element = NULL;
            httpdns_cJSON_ArrayForEach(a_element, a)
            {
                /* TODO This has O(n^2) runtime, which is horrible! */
                b_element = get_object_item(b, a_element->string, case_sensitive);
                if (b_element == NULL)
                {
                    return false;
                }

                if (!httpdns_cJSON_Compare(a_element, b_element, case_sensitive))
                {
                    return false;
                }
            }

            /* doing this twice, once on a and b to prevent true comparison if a subset of b
             * TODO: Do this the proper way, this is just a fix for now */
            httpdns_cJSON_ArrayForEach(b_element, b)
            {
                a_element = get_object_item(a, b_element->string, case_sensitive);
                if (a_element == NULL)
                {
                    return false;
                }

                if (!httpdns_cJSON_Compare(b_element, a_element, case_sensitive))
                {
                    return false;
                }
            }

            return true;
        }

        default:
            return false;
    }
}

HTTPDNS_CJSON_PUBLIC(void *) httpdns_cJSON_malloc(size_t size)
{
    return global_hooks.allocate(size);
}

HTTPDNS_CJSON_PUBLIC(void) httpdns_cJSON_free(void *object)
{
    global_hooks.deallocate(object);
}
