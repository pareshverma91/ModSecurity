// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "ri_api_internal.h"
#include "ri_re2.h"
#include <re2/re2.h>
#include <utility>
#include <string.h>

/**
** Translate RI options to RE2 options.
** @param pattern:     The regular expression for compilation.
** @param options:     The RI options for compilation.
** @param re2_pattern: The regular expression for RE2's compilation.
** @param re2_options: The options for RE2's compilation.
**/
static void ri_re2_translate_comp_options(const char *pattern, int options,
        std::string &re2_pattern, RE2::Options &re2_options);

/**
** Compile the provided regular expression pattern.
** @param re:      A pointer to RE2 object if compilation succeeds,
**                 or NULL if compilation fails.
** @param pattern: The regular expression for compilation.
** @param options: The options for compilation.
** @param log:     The pointer to the log. It is optional.
**                 If it is NULL, we would not fill in detailed log message.
** return: 0 if compilation succeeds,
**         or the error code if compilation fails.
**/
int ri_re2_comp(void **re, const char *pattern, int options, const char **log)
{
    int error_code = 0;
    RE2 *re2_re = NULL;
    std::string re2_pattern;
    RE2::Options re2_options;

    if (pattern == NULL) {
        error_code = RI_ERROR_PATTERN_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    ri_re2_translate_comp_options(pattern, options,
            re2_pattern, re2_options);

    // Construct RE2 object
    re2_re = new RE2(re2_pattern, re2_options);

    if (re2_re == NULL) {
        error_code = RI_ERROR_RE2_CANNOT_CONSTRUCT;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    } else {
        if (re2_re->ok()) {
            *re = re2_re;
            return error_code;
        } else {
            ri_fill_log_ex(log, RI_LOG_ERROR, re2_re->error().c_str());
            ri_re2_free(re2_re);
            error_code = RI_ERROR_RE2_CANNOT_CONSTRUCT;
            return error_code;
        }
    }
}

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
        start_offset, int *ovector, int ovec_size)
{
    RE2 *re2_re = static_cast<RE2 *>(re);
    size_t start_pos = start_offset;
    const size_t end_pos = subject_len;
    // Total number of sub-matches in the regex pattern
    int num_submatch = 1 + re2_re->NumberOfCapturingGroups();
    re2::StringPiece submatches[num_submatch];
    // Index of the last NULL sub-match
    int last_null_submatch = num_submatch - 1;

    // If the string does not match the pattern
    if (!re2_re->Match(subject, start_pos, end_pos, RE2::UNANCHORED,
                submatches, num_submatch))
        return -1;

    // Find the last NULL sub-match
    while (!submatches[last_null_submatch].data()) {
        last_null_submatch--;
    }

    int count = std::min(last_null_submatch + 1, ovec_size / 3);
    // Extract sub-match information as much as possible
    for (int i = 0; i < count; i++) {
        if (!submatches[i].data()) {
            ovector[2 * i] = -1;
            ovector[2 * i + 1] = -1;
        } else {
            ovector[2 * i] = submatches[i].data() - subject;
            ovector[2 * i + 1] = ovector[2 * i] + submatches[i].length();
        }
    }

    // The output vector has enough space to store the information of
    // all valid sub-matches and NULL sub-matches among valid sub-matches
    if (last_null_submatch + 1 <= ovec_size / 3)
        return last_null_submatch + 1;

    // Truncate NULL sub-matches at the tail of 'ovector'
    // if (!submatches[ovec_size / 3 - 1].data()) {
    //     for (int i = ovec_size / 3 - 2; i >= 0; i--) {
    //         if (submatches[i].data()) {
    //             return i + 1;
    //         }
    //     }
    // }
    return 0;
}

/**
** Free the compiled regex.
** @param re: The pointer to the compiled regex of RE2.
**/
void ri_re2_free(void *re)
{
    delete (RE2 *) re;
}

/**
** Translate RI options to RE2 options.
** @param pattern:     The regular expression for compilation.
** @param options:     The RI options for compilation.
** @param re2_pattern: The regular expression for RE2's compilation.
** @param re2_options: The options for RE2's compilation.
**/
static void ri_re2_translate_comp_options(const char *pattern, int options,
        std::string &re2_pattern, RE2::Options &re2_options)
{
    re2_pattern.assign(pattern, strlen(pattern));

    // Use Latin-1 encoding which processes the string as one-byte characters
    re2_options.set_encoding(RE2::Options::EncodingLatin1);

    // We store the error message in the log instead of printing it out
    re2_options.set_log_errors(false);

    if (options & RI_COMP_CASELESS) {
        re2_options.set_case_sensitive(false);
    }

    if (options & RI_COMP_DOTALL) {
        re2_options.set_dot_nl(true);
    }

    // To enable MULTILINE options, modify the pattern to (?m:pattern)
    if (options & RI_COMP_MULTILINE) {
        std::string prefix = "(?m:";
        std::string t_pattern(pattern);
        std::string postfix = ")";
        re2_pattern = prefix + t_pattern + postfix;
    }
}
