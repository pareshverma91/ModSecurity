/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
#ifndef _RI_API_INTERNAL_H_
#define _RI_API_INTERNAL_H_

#include "ri_api.h"

struct ri_regex {
    char *pattern;             // The regex pattern
    void *pcre_re;             // pcre*, compiled PCRE regex
    void *pcre_pe;             // pcre_extra* for PCRE
    void *pcre_jit_re;         // pcre*, compiled PCRE regex
    void *pcre_jit_pe;         // pcre_extra* for PCRE-JIT
    void *re2_re;              // RE2*, compiled RE2 regex
    struct ri_priority priority; // The priority of regex engines for the pattern
};

#ifdef __cplusplus
extern "C" {
#endif

/**
** Fill in log according on the error_code
** @param log:        The pointer to the log.
** @param level:      The log level to fill the detailed error information.
** @param error_code: The error code.
** return: RI_SUCCESS if it succeeds, or the error code.
**/
extern int ri_fill_log(const char ** log,  ri_log_level_t log_level,
        int error_code);

/**
** Fill in log according on the format and rested paramenters
** @param log:        The pointer to the log.
** @param level:      The log level to fill the detailed error information.
** @param error_code: The error code.
** @param format:     The format string to record log, like printf.
** return: RI_SUCCESS if it succeeds, or the error code.
**/
extern int ri_fill_log_ex(const char ** log, ri_log_level_t log_level,
        const char * format, ...);

#ifdef __cplusplus
}
#endif

#endif /* _RI_API_INTERNAL_H_ */
