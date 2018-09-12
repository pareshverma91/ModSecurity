/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef _MSC_RI_H_
#define _MSC_RI_H_

typedef struct msc_ri_regex_t msc_ri_regex_t;

#include "apr_general.h"
#include "modsecurity.h"


#include <regex_integrator/ri_api.h>

struct msc_ri_regex_t {
    ri_regex_t       re;
    const char      *pattern;
};

extern unsigned int g_use_regex_integrator;

/* Compiles the provided regular expression pattern. */
msc_ri_regex_t * msc_ri_pregcomp_ex(apr_pool_t *pool, const char *pattern, int options,
        const char **_errptr, int match_limit, int match_limit_recursion);

/* Compiles the provided regular expression pattern.  Calls msc_ri_pregcomp_ex()
 * with default limits. */
msc_ri_regex_t * msc_ri_pregcomp(apr_pool_t *pool, const char *pattern, int options,
        const char **_errptr);

/* Executes regular expression with extended options. */
int msc_ri_regexec_ex(msc_ri_regex_t * regex, const char *s,
        unsigned int slen, int startoffset, int options,
        int *ovector, int ovecsize, char **error_msg);

/* Executes regular expression, capturing subexpressions in the given vector. */
int msc_ri_regexec_capture(msc_ri_regex_t * regex, const char *s,
        unsigned int slen, int *ovector,
        int ovecsize, char **error_msg);

/* Executes regular expression but ignores any of the subexpression captures. */
int msc_ri_regexec(msc_ri_regex_t * regex, const char *s,
        unsigned int slen, char **error_msg);

#endif
