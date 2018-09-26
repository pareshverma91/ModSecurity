/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef _MSC_REGEX_H_
#define _MSC_REGEX_H_

#include "msc_pcre.h"

#ifdef REGEX_INTEGRATOR

#include "msc_ri.h"

extern unsigned int g_use_regex_integrator;
extern struct ri_priority g_default_ri_priority;

#endif

/* Compiles the provided regular expression pattern. */
void * msc_regex_pregcomp_ex(apr_pool_t *pool, const char *pattern, int options,
        const char **_errptr, int *_erroffset,
        int match_limit, int match_limit_recursion);

/* Compiles the provided regular expression pattern.  Calls msc_regex_pregcomp_ex()
 * with default limits. */
void * msc_regex_pregcomp(apr_pool_t *pool, const char *pattern, int options,
        const char **_errptr, int *_erroffset);

/* Executes regular expression with extended options. */
int msc_regex_regexec_ex(const void * regex, const char *s,
        unsigned int slen, int startoffset, int options,
        int *ovector, int ovecsize, char **error_msg);

/* Executes regular expression, capturing subexpressions in the given vector. */
int msc_regex_regexec_capture(const void * regex, const char *s,
        unsigned int slen, int *ovector,
        int ovecsize, char **error_msg);

/* Executes regular expression but ignores any of the subexpression captures. */
int msc_regex_regexec(const void * regex, const char *s,
        unsigned int slen, char **error_msg);

/* Gets info on a compiled regex. */
int msc_regex_fullinfo(const void * regex, int what, void *where);

/* Gets pattern on a compiled regex. */
const char * msc_regex_pattern(const void * regex);

#endif
