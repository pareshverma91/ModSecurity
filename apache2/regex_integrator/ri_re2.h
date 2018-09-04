// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#ifndef _RI_RE2_H_
#define _RI_RE2_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "ri_error.h"

/**
** Compile the provided regular expression pattern.
** @param pattern: The regular expression for compilation.
** @param options: The options for compilation.
** @param log:     The pointer to the log. It is optional.
**                 If it is NULL, we would not fill in detailed log message.
** return: A pointer to RE2 object if compilation succeeds,
**         or NULL if compilation fails.
**/
void *ri_re2_comp(const char *pattern, int options, const char ** log);

/**
** Execute regular expression.
** @param re:           The pointer to the compiled regex of RE2.
** @param subject:      The pointer to the subject string.
** @param subject_len:  The length of the subject string.
** @param start_offset: The offset in the subject at which to start matching.
** @param ovector:      The pointer to a vector storing the sub-matches.
** @param ovec_size:    The number of elements in the vector (a multiple of 3).
** return: The number of captured sub-matches,
**         or 0 if the subject matches but the output vector is not big enough,
**         or -1 if there is no match.
**/
int ri_re2_exec(void *re, const char *subject, unsigned int subject_len, int
        start_offset, int *ovector, int ovec_size);

/**
** Free the compiled regex.
** @param re: The pointer to the compiled regex of RE2.
**/
void ri_re2_free(void *re);

#ifdef __cplusplus
}
#endif

#endif /* _RI_RE2_H_ */
