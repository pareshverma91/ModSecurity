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

#include "msc_regex.h"

unsigned int g_use_regex_integrator = 0;

/**
 * Compiles the provided regular expression pattern. The _err*
 * parameters are optional, but if they are provided and an error
 * occurs they will contain the error message and the offset in
 * the pattern where the offending part of the pattern begins. The
 * match_limit* parameters are optional and if >0, then will set
 * match limits.
 */
void * msc_regex_pregcomp_ex(apr_pool_t *pool, const char *pattern, int options,
                      const char **_errptr, int *_erroffset,
                      int match_limit, int match_limit_recursion)
{
    if (g_use_regex_integrator) {
        return msc_ri_pregcomp_ex(pool, pattern, options, _errptr, match_limit, match_limit_recursion);
    } else {
        return msc_pregcomp_ex(pool, pattern, options, _errptr, _erroffset, match_limit, match_limit_recursion);
    }
}

/**
 * Compiles the provided regular expression pattern.  Calls msc_pregcomp_ex()
 * with default limits.
 */
void * msc_regex_pregcomp(apr_pool_t *pool, const char *pattern, int options,
                   const char **_errptr, int *_erroffset)
{
    if (g_use_regex_integrator) {
        return msc_ri_pregcomp(pool, pattern, options, _errptr);
    } else {
        return msc_pregcomp(pool, pattern, options, _errptr, _erroffset);
    }
}

/**
 * Executes regular expression with extended options.
 * Returns PCRE_ERROR_NOMATCH when there is no match, error code < -1
 * on errors, and a value > 0 when there is a match.
 */
int msc_regex_regexec_ex(const void * regex, const char *s, unsigned int slen,
    int startoffset, int options, int *ovector, int ovecsize, char **error_msg)
{
    if (g_use_regex_integrator) {
        return msc_ri_regexec_ex((msc_ri_regex_t *)regex, s, slen, startoffset, options, ovector, ovecsize, error_msg);
    } else {
        return msc_regexec_ex((msc_regex_t *)regex, s, slen, startoffset, options, ovector, ovecsize, error_msg);
    }
}

/**
 * Executes regular expression, capturing subexpressions in the given
 * vector. Returns PCRE_ERROR_NOMATCH when there is no match, error code < -1
 * on errors, and a value > 0 when there is a match.
 */
int msc_regex_regexec_capture(const void * regex, const char *s, unsigned int slen,
    int *ovector, int ovecsize, char **error_msg)
{
    if (g_use_regex_integrator) {
        return msc_ri_regexec_capture((msc_ri_regex_t *)regex, s, slen, ovector, ovecsize, error_msg);
    } else {
        return msc_regexec_capture((msc_regex_t *)regex, s, slen, ovector, ovecsize, error_msg);
    }
}

/**
 * Executes regular expression but ignores any of the subexpression
 * captures. See above for the return codes.
 */
int msc_regex_regexec(const void * regex, const char *s, unsigned int slen,
    char **error_msg)
{
    if (g_use_regex_integrator) {
        return msc_ri_regexec((msc_ri_regex_t *)regex, s, slen, error_msg);
    } else {
        return msc_regexec((msc_regex_t *)regex, s, slen, error_msg);
    }
}

/**
 * Gets info on a compiled regex.
 */
int msc_regex_fullinfo(const void * regex, int what, void *where)
{
    if (g_use_regex_integrator) {
        return -1000;   /* To differentiate from PCRE as it already uses -1. */
    } else {
        return msc_fullinfo((msc_regex_t *)regex, what, where);
    }
}

const char * msc_regex_pattern(const void * regex) {
    if (g_use_regex_integrator) {
        return ((msc_ri_regex_t * )regex)->pattern;
    } else {
        return ((msc_regex_t *)regex)->pattern;
    }
}
