/*
* ModSecurity for Apache 2.x, http://www.modsecurity.org/
* Copyright (c) 2004-2013 Trustwave Holdings, Inc. (http://www.trustwave.com/)
*
* You may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* If any of the files related to licensing are missing or if you have any
* other questions related to licensing please contact Trustwave Holdings, Inc.
* directly using the email address security@modsecurity.org.
*/

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

/* Compiles the provided regular expression pattern.  Calls msc_pregcomp_ex()
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
