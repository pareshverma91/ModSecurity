/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
#include "ri_api_internal.h"
#include "ri_pcre.h"
#include "pcre.h"
#include <string.h>
#include <stdio.h>

/**
** Translate RI's options to PCRE's compilation options.
** @param options: The RI options for compilation.
**/
static int ri_pcre_translate_comp_options(int options);

/**
** Translate RI's options to PCRE's execution options.
** @param options: The RI options for execution.
**/
static int ri_pcre_translate_exec_options(int options);

/**
** Compile the provided regular expression pattern.
** @param re:                    The pointer to the compiled regex.
** @param pe:                    The pointer to the studied information.
**                               It is optional.
**                               We perform PCRE study when it is not NULL.
** @param pattern:               The regular expression for compilation.
** @param options:               The compilation options.
** @param match_limit:           The limit on internal resource use.
**                               It is optional. It takes effect when it is > 0.
** @param match_limit_recursion: The limit on internal recursion depth.
**                               It is optional. It takes effect when it is > 0.
** @param use_pcre_jit:          The indicator to enable PCRE-JIT.
**                               0 means disabling PCRE-JIT;
**                               otherwise, enable PCRE-JIT.
** @param log:                   The pointer to the log. It is optional.
**                               If it is NULL, we would not fill in detailed log message.
** return: 0 if compilation succeeds,
**         or the error code if compilation fails.
**/
int ri_pcre_comp(void **re, void **pe,
        const char *pattern, int options,
        int match_limit, int match_limit_recursion,
        int use_pcre_jit, const char **log)
{
    int error_code = 0;
    const char *error_ptr = NULL;
    int error_offset = 0;

    if (pattern == NULL) {
        error_code = RI_ERROR_PATTERN_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    if (re == NULL) {
        error_code = RI_ERROR_PCRE_NULL_POINTER;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    // Translate options to PCRE's options
    int pcre_comp_options = ri_pcre_translate_comp_options(options);

    // PCRE Compile
    // If *re is not NULL, we would not do compilation
    if (*re == NULL) {
        *re = pcre_compile(pattern, pcre_comp_options,
                &error_ptr, &error_offset, NULL);
        if (*re == NULL) {
            ri_fill_log_ex(log, RI_LOG_ERROR, "%s, error offset = %d", error_ptr, error_offset);
            return RI_ERROR_PCRE_COMPILATION_FAILURE;
        }
    }

    // If pe is NULL, then no need to call pcre_study()
    if (pe == NULL) {
        return RI_SUCCESS;
    }

    // If *pe is not NULL, we would not do study
    if (*pe == NULL) {
        *pe = use_pcre_jit 
            ? pcre_study((pcre *) *re, PCRE_STUDY_JIT_COMPILE, &error_ptr)
            : pcre_study((pcre *) *re,0 , &error_ptr);

        // Set up the pcre_extra record if pcre_study did not do it
        if (*pe == NULL) {
            *pe = (pcre_extra *) pcre_malloc(sizeof(pcre_extra));
            if (*pe == NULL) {
                error_code = RI_ERROR_PCRE_MALLOC;
                ri_fill_log(log, RI_LOG_ERROR, error_code);
                return error_code;
            }
            memset(*pe, 0, sizeof(pcre_extra));
        }

        pcre_extra *t_pe = (pcre_extra *) *pe;

#ifdef PCRE_EXTRA_MATCH_LIMIT
        // If match limit is available, then use it
        if (match_limit > 0) {
            t_pe->match_limit = match_limit;
            t_pe->flags |= PCRE_EXTRA_MATCH_LIMIT;
        }
#else
#pragma message ("This PCRE version does not support match limits! Upgrade to \
        at least PCRE v6.5.")
#endif /* PCRE_EXTRA_MATCH_LIMIT */

#ifdef PCRE_EXTRA_MATCH_LIMIT_RECURSION
        // If match limit recursion is available, then use it
        if (match_limit_recursion > 0) {
            t_pe->match_limit_recursion = match_limit_recursion;
            t_pe->flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
        }
#else
#pragma message ("This PCRE version does not support match recursion limits! \
        Upgrade to at least PCRE v6.5.")
#endif /* PCRE_EXTRA_MATCH_LIMIT_RECURSION */
    }

    return RI_SUCCESS;
}

/**
** Execute regular expression.
** @param re:           The pointer to the compiled regex.
** @param pe:           The pointer to an associated pcre_extra structure.
** @param subject:      The pointer to the subject string.
** @param subject_len:  The length of the subject string.
** @param start_offset: The offset in the subject at which to start matching.
** @param options:      The execution options.
** @param ovector:      The pointer to a vector storing sub-matches.
** @param ovec_size:    The number of elements in the vector (a multiple of 3).
** return: The number of captured sub-matches,
**         or 0 if the subject matches but the output vector is not big enough,
**         or -1 if there is no match,
**         or error code (< -1) if error occurs.
**/
int ri_pcre_exec(const void *re, const void *pe, const char *subject,
        unsigned int subject_len, int start_offset,
        int options, int *ovector, int ovec_size)
{
    int pcre_exec_options = ri_pcre_translate_exec_options(options);

    return pcre_exec((pcre *) re, (pcre_extra *) pe, subject, subject_len,
            start_offset, pcre_exec_options, ovector, ovec_size);
}

/**
** Free compiled pcre regex and study data.
** @param re: The pointer to a compiled regex.
** @param pe: The pointer to an associated pcre_extra structure.
**/
void ri_pcre_free(void *re, void *pe)
{
    if (re != NULL)
        pcre_free((pcre *) re);

    // This function was added to the API for release 8.20.
    if (pe != NULL)
        pcre_free_study((pcre_extra *) pe);
}

/**
** Translate RI's options to PCRE's compilation options.
** @param options: The RI options for compilation.
**/
static int ri_pcre_translate_comp_options(int options)
{
    int pcre_options = 0;
    if (options & RI_COMP_CASELESS)
        pcre_options |= PCRE_CASELESS;

    if (options & RI_COMP_MULTILINE)
        pcre_options |= PCRE_MULTILINE;

    if (options & RI_COMP_DOTALL)
        pcre_options |= PCRE_DOTALL;

    if (options & RI_COMP_DOLLAR_ENDONLY)
        pcre_options |= PCRE_DOLLAR_ENDONLY;

    return pcre_options;
}

/**
** Translate RI's options to PCRE's execution options.
** @param options: The RI options for execution.
**/
static int ri_pcre_translate_exec_options(int options)
{
    int pcre_options = 0;

    if (options & RI_EXEC_NOTEMPTY)
        pcre_options |= PCRE_NOTEMPTY;

    return pcre_options;
}
