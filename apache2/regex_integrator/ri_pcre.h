/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
#ifndef _RI_PCRE_H_
#define _RI_PCRE_H_

#include "ri_error.h"

/**
** Compile the provided regular expression pattern.
** @param re:                    The pointer to the compiled regex.
** @param pe:                    The pointer to the studied information.
**                               It is optional.
**                               We perform PCRE study when it is not NULL.
** @param pattern:               The regular expression for compilation.
** @param options:               The compilation options.
** @param params:                The params needed by PCRE.
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
        const struct ri_pcre_params * params,
        int use_pcre_jit, const char **log);

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
        int options, int *ovector, int ovec_size);

/**
** Free compiled pcre regex and study data.
** @param re: The pointer to a compiled regex.
** @param pe: The pointer to an associated pcre_extra structure.
**/
void ri_pcre_free(void *re, void *pe);

#endif /*_RI_PCRE_H */
