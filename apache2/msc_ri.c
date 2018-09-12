/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifdef REGEX_INTEGRATOR

#include "msc_ri.h"


/**
 * Releases the resources used by a single regular expression pattern.
 */
static apr_status_t msc_ri_cleanup(msc_ri_regex_t * regex) {
    if (regex != NULL) {
        if (regex->re) {
            ri_free(regex->re);
        }
    }
    return APR_SUCCESS;
}

/**
 * Compiles the provided regular expression pattern. The _err*
 * parameters are optional, but if they are provided and an error
 * occurs they will contain the error message and the offset in
 * the pattern where the offending part of the pattern begins. The
 * match_limit* parameters are optional and if >0, then will set
 * match limits.
 */
msc_ri_regex_t * msc_ri_pregcomp_ex(apr_pool_t *pool, const char *pattern, int options,
                      const char **_errptr,
                      int match_limit, int match_limit_recursion)
{
    int error_code = 0;
    int ri_options = RI_COMP_DEFAULT;
    const static struct ri_priority priority = {
        {RI_RE2, RI_PCRE_JIT, RI_PCRE},
        3,
    };
    const struct ri_params params = {
        // Set PCRE params
        {
            match_limit, match_limit_recursion,
        },
        //SET PCRE-JIT params
        {    match_limit, match_limit_recursion,},
    };
    msc_ri_regex_t * regex = NULL;
    regex = apr_pcalloc(pool, sizeof(msc_ri_regex_t));
    regex->pattern = pattern;
    // Set options
    if (options & PCRE_CASELESS)
        ri_options |= RI_COMP_CASELESS;

    if (options & PCRE_MULTILINE)
        ri_options |= RI_COMP_MULTILINE;

    if (options & PCRE_DOTALL)
        ri_options |= RI_COMP_DOTALL;

    if (options & PCRE_DOLLAR_ENDONLY)
        ri_options |= RI_COMP_DOLLAR_ENDONLY;

    error_code = ri_create(&(regex->re), pattern, ri_options, 
            &priority, &params, _errptr);

    if (error_code != RI_SUCCESS)
        return NULL;
        
    apr_pool_cleanup_register(pool, (void *)regex,
        (apr_status_t (*)(void *))msc_ri_cleanup, apr_pool_cleanup_null);

    return regex;
}

/**
 * Compiles the provided regular expression pattern.  Calls msc_ri_pregcomp_ex()
 * with default limits.
 */
msc_ri_regex_t * msc_ri_pregcomp(apr_pool_t *pool, const char *pattern, int options,
                   const char **_errptr)
{
    return msc_ri_pregcomp_ex(pool, pattern, options, _errptr, 0, 0);
}

/**
 * Executes regular expression with extended options.
 * Returns PCRE_ERROR_NOMATCH when there is no match, error code < -1
 * on errors, and a value > 0 when there is a match.
 */
int msc_ri_regexec_ex(msc_ri_regex_t * regex, const char *s, unsigned int slen,
    int startoffset, int options, int *ovector, int ovecsize, char **error_msg)
{
    int ri_options = RI_EXEC_DEFAULT;
    int rv = 0;

    if (error_msg == NULL)
         return -1000; /* To differentiate from PCRE as it already uses -1. */
    *error_msg = NULL;

    if (options & PCRE_NOTEMPTY)
        ri_options |= RI_EXEC_NOTEMPTY;

    return ri_exec(regex->re, s, slen, startoffset, ri_options,
            NULL, ovector, ovecsize, (const char **)error_msg);
}

/**
 * Executes regular expression, capturing subexpressions in the given
 * vector. Returns PCRE_ERROR_NOMATCH when there is no match, error code < -1
 * on errors, and a value > 0 when there is a match.
 */
int msc_ri_regexec_capture(msc_ri_regex_t * regex, const char *s, unsigned int slen,
    int *ovector, int ovecsize, char **error_msg)
{
    if (error_msg == NULL) return -1000; /* To differentiate from PCRE as it already uses -1. */
    *error_msg = NULL;

    return msc_ri_regexec_ex(regex, s, slen, 0, 0, ovector, ovecsize, error_msg);
}

/**
 * Executes regular expression but ignores any of the subexpression
 * captures. See above for the return codes.
 */
int msc_ri_regexec(msc_ri_regex_t * regex, const char *s, unsigned int slen,
    char **error_msg)
{
    if (error_msg == NULL) return -1000; /* To differentiate from PCRE as it already uses -1. */
    *error_msg = NULL;

    return msc_ri_regexec_ex(regex, s, slen, 0, 0, NULL, 0, error_msg);
}

#endif
