/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
#include "ri_api_internal.h"
#include "ri_pcre.h"
#include "ri_re2.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

// Default priority
static const struct ri_priority RI_PRIORITY_DEFAULT = {
    {
        RI_RE2, 
        RI_PCRE_JIT, 
        RI_PCRE
    },
    3
};

// Default param
static const struct ri_pcre_params RI_PCRE_PARAMS_DEFAULT= {
    0,
    0,
};

// Global log level
static ri_log_level_t g_ri_log_level = RI_LOG_ERROR;

// Global log message
static __thread char *g_ri_log_msg = NULL;
// Global log buffer length
static size_t g_ri_log_len = 0;

#define MAX(a, b)               \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a, b)               \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/** Update priority_dst using priority_src.
** @param priority_dst:     The priority to update.
** @param priority_src:     The priority used to update.
** @param pattern:          The regex expression for profiling in RI_AUTO mode.
** @param comp_comptions:   Compilation options for profiling in RI_AUTO mode.
** @param exec_comptions:   Execution options for profiling in RI_AUTO mode.
** @param log:              The pointer to the log. It is optional.
**                          We would not output the log message if it is NULL.
** return: 0 if it succeeds,
**         or the error code.
**/
static int ri_update_priority(struct ri_priority *priority_dst,
        const struct ri_priority *priority_src,
        const char *pattern, int comp_options, int exec_options,
        const char **log);

/**
** Check whether the priority is valid.
** @param priority:     The pointer to the priority.
** @param log:          The pointer to the log. It is optional.
**                      We would not output the log message if it is NULL.
** return: 0 if the priority is valid,
**         or the error code.
**/
static int ri_validate_priority(const struct ri_priority *priority,
        const char **log);

/**
** Perform profiling to figure out the optimal priority of regex engines.
** @param pattern:      The pointer to the pattern.
** @param comp_options: Compilation options.
** @param exec_options: Execution options.
** @param priority:     The pointer to the priority output by profiling.
** @param log:          The pointer to the log. It is optional.
**                      We would not output the log message if it is NULL.
** return: 0 if the profiling succeeds,
**         or the error code.
**/
static int ri_do_profiling(const char *pattern, int comp_options, 
        int exec_options, struct ri_priority *priority, 
        const char **log);

/**
** Compile the provided regular expression pattern.
** @param regex:        The pointer to the compiled regex.
** @param pattern:      The regular expression for compilation.
** @param options:      The compilation options.
**                      It contains various bit settings of ri_comp_option.
**                      Its usage is as same as the bitmap.
** @param priority:     The pointer to the priority of regex engines.
**                      It is optional, NULL is using default priority.
** @param params:       The pointer to additional parameters for compilation.
**                      It is optional, NULL is using default params.
** @param log:          The pointer to the log. It is optional.
**                      We would not output the log message if it is NULL.
** return: 0 if compilation succeeds,
**         or the error code.
**/
int ri_create(ri_regex_t *regex, const char *pattern, int options,
        const struct ri_priority *priority, const struct ri_params *params,
        const char **log)
{
    int error_code = 0;
    struct ri_regex *t_regex = NULL;
    int match_limit = 0;
    int match_limit_recursion = 0;

    if (regex == NULL) {
        error_code = RI_ERROR_REGEX_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    if (pattern == NULL) {
        error_code = RI_ERROR_PATTERN_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    *regex = calloc(1, sizeof(struct ri_regex));
    if (*regex == NULL) {
        error_code = RI_ERROR_CANNOT_ALLOCATE_MEMORY_TO_REGEX;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    t_regex = (struct ri_regex *) *regex;

    // Set regex's pattern
    t_regex->pattern = (char *) malloc(strlen(pattern) + 1);
    if (t_regex->pattern == NULL) {
        ri_free(*regex);
        *regex = NULL;
        error_code = RI_ERROR_CANNOT_ALLOCATE_MEMORY_TO_REGEX;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }
    strcpy(t_regex->pattern, pattern);

    // Set regex's priority
    error_code = ri_update_priority(&t_regex->priority,
            priority, t_regex->pattern, options, 0, log);
    if (error_code != RI_SUCCESS) {
        ri_free(*regex);
        *regex = NULL;
        return error_code;
    }

    // PCRE compile
    if (params == NULL) {
        match_limit = RI_PCRE_PARAMS_DEFAULT.match_limit;
        match_limit_recursion = RI_PCRE_PARAMS_DEFAULT.match_limit_recursion;
    } else {
        match_limit = params->pcre.match_limit;
        match_limit_recursion = params->pcre.match_limit_recursion;
    }
    error_code = ri_pcre_comp(&(t_regex->pcre_re), &(t_regex->pcre_pe),
        t_regex->pattern, options, match_limit, match_limit_recursion,
        0, log);
    if (error_code != RI_SUCCESS) {
        ri_free(*regex);
        *regex = NULL;
        return error_code;
    }

    // PCRE-JIT compile
    if (params == NULL) {
        match_limit = RI_PCRE_PARAMS_DEFAULT.match_limit;
        match_limit_recursion = RI_PCRE_PARAMS_DEFAULT.match_limit_recursion;
    } else {
        match_limit = params->pcre_jit.match_limit;
        match_limit_recursion = params->pcre_jit.match_limit_recursion;
    }
    error_code = ri_pcre_comp(&(t_regex->pcre_re), &(t_regex->pcre_jit_pe),
        t_regex->pattern, options, match_limit, match_limit_recursion,
        1, log);
    if (error_code != RI_SUCCESS) {
        ri_free(*regex);
        *regex = NULL;
        return error_code;
    }

    // RE2 compile
    t_regex->re2_re = ri_re2_comp(t_regex->pattern, options, log);

    error_code = RI_SUCCESS;
    ri_fill_log(log, RI_LOG_ERROR, error_code);
    return error_code;
}

/**
** Execute regular expression.
** @param regex:        The compiled regex.
** @param subject:      The pointer to the subject string.
** @param subject_len:  The length of the subject string.
** @param start_offset: The offset in the subject at which to start matching.
** @param options:      The execution options.
**                      It contains various bit settings of ri_exec_option.
**                      Its usage is as same as the bitmap.
** @param priority:     The priority of regex engines for execution.
**                      It is optional.
**                      If it is NULL, use the priority in the regex.
**                      It does not override the priority in the regex.
** @param ovector:      The pointer to a vector storing sub-matches.
**                      Please refer to the explanation of arguments in pcre_exec().
**                      It is optional.
** @param ovec_size:    Number of elements in the vector (a multiple of 3).
**                      Please refer to the explanation of arguments in pcre_exec().
** @param log:          The pointer to the log. It is optional.
**                      We would not output the log message if it is NULL.
** return: The number of captured sub-matches,
**         or 0 if the subject matches but the output vector is not big enough,
**         or -1 if there is no match,
**         or error code (< -1) if error occurs.
**/
int ri_exec(const ri_regex_t regex, const char *subject,
        unsigned int subject_len, int start_offset, int options,
        const struct ri_priority *priority,
        int *ovector, int ovec_size, const char **log)
{
    int error_code = 0;
    struct ri_regex *t_regex = NULL;
    // The priority for execution
    struct ri_priority priority_exec = RI_PRIORITY_DEFAULT;
    // Returned value of RE2
    int rv_re2 = 0;

    if (regex == NULL) {
        error_code = RI_ERROR_REGEX_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    if (subject == NULL) {
        error_code = RI_ERROR_EXEC_SUBJECT_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    t_regex = (struct ri_regex *) regex;

    // Get the priority for execution
    if (priority == NULL) {
        priority_exec = t_regex->priority;
    } else {
        error_code = ri_update_priority(&priority_exec,
                priority, t_regex->pattern, 0, 0, log);
        if (error_code != RI_SUCCESS)
            return error_code;
    }

    for (unsigned int i = 0; i < priority_exec.engine_count; i++) {
        switch (priority_exec.engines[i]) {
        case RI_PCRE:
            if (t_regex->pcre_re != NULL)
                return ri_pcre_exec(t_regex->pcre_re, t_regex->pcre_pe, subject,
                        subject_len, start_offset, options, ovector, ovec_size);
            break;
        case RI_PCRE_JIT:
            if (t_regex->pcre_re != NULL)
                return ri_pcre_exec(t_regex->pcre_re, t_regex->pcre_jit_pe,
                        subject, subject_len, start_offset, options,
                        ovector, ovec_size);
            break;
        case RI_RE2:
            if (t_regex->re2_re != NULL) {
                rv_re2 = ri_re2_exec(t_regex->re2_re, subject, subject_len,
                        start_offset, ovector, ovec_size);
                // When the string matches the regex and EXEC_NOTEMPTY is on,
                // use PCRE to do matching again if:
                // 1) ovector == NULL;
                // 2) the match is an empty string, i.e., ovector[1] == ovector[0]
                // Otherwise, return the result.
                if (!(rv_re2 >= 0 && (options & RI_EXEC_NOTEMPTY) &&
                      (!ovector || ovector[1] == ovector[0]))) {
                    return rv_re2;
                }
            }
            break;
        case RI_AUTO:
        default:
            error_code = RI_ERROR_EXEC_INVALID_REGEX_ENGINE;
            ri_fill_log(log, RI_LOG_ERROR, error_code);
            return error_code;
        }
    }

    error_code = RI_ERROR_EXEC_NO_USABLE_REGEX_ENGINE;
    ri_fill_log(log, RI_LOG_ERROR, error_code);
    return error_code;
}

/**
** Free the compiled regex.
** @param regex: The compiled regex.
**/
void ri_free(ri_regex_t regex)
{
    if (regex != NULL) {
        struct ri_regex *t_regex = (struct ri_regex *) regex;

        if (t_regex->pattern != NULL) {
            free(t_regex->pattern);
            t_regex->pattern = NULL;
        }

        if (t_regex->pcre_re != NULL) {
            ri_pcre_free(t_regex->pcre_re, NULL);
            t_regex->pcre_re = NULL;
        }

        if (t_regex->pcre_pe != NULL) {
            ri_pcre_free(NULL, t_regex->pcre_pe);
            t_regex->pcre_pe = NULL;
        }

        if (t_regex->pcre_jit_pe != NULL) {
            ri_pcre_free(NULL, t_regex->pcre_jit_pe);
            t_regex->pcre_jit_pe = NULL;
        }

        if (t_regex->re2_re != NULL) {
            ri_re2_free(t_regex->re2_re);
            t_regex->re2_re = NULL;
        }

        free(t_regex);
        t_regex = NULL;
    }
}

/**
** Set regex's priority using the new priority.
** @param regex:            The compiled regex.
** @param priority_new:     The pointer to the new priority.
**                          If it is NULL, set default priority for the regex.
** @param log:              The pointer to the log. It is optional.
**                          We would not output the log message if it is NULL.
** return: 0 if it succeeds,
**         or the error code.
**/
int ri_set_priority(ri_regex_t regex, 
        const struct ri_priority * priority_new, 
        const char ** log)
{
    struct ri_regex *t_regex = NULL;

    if (regex == NULL)
        return RI_ERROR_REGEX_NULL;

    t_regex = (struct ri_regex *) regex;

    return ri_update_priority(&(t_regex->priority),
            priority_new, t_regex->pattern, 0, 0, log);
}

/** Update priority_dst using priority_src.
** @param priority_dst:     The priority to update.
** @param priority_src:     The priority used to update.
** @param pattern:          The regex expression for profiling in RI_AUTO mode.
** @param comp_comptions:   Compilation options for profiling in RI_AUTO mode.
** @param exec_comptions:   Execution options for profiling in RI_AUTO mode.
** @param log:              The pointer to the log. It is optional.
**                          We would not output the log message if it is NULL.
** return: 0 if it succeeds,
**         or the error code.
**/
static int ri_update_priority(struct ri_priority * priority_dst,
        const struct ri_priority *priority_src,
        const char *pattern, int comp_options, int exec_options, 
        const char **log)
{
    int error_code = 0;

    if (priority_dst == NULL) {
        error_code = RI_ERROR_PRIORITY_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    if (priority_src != NULL) {
        // Validate the priority
        error_code = ri_validate_priority(priority_src, log);
        if (error_code != RI_SUCCESS)
            return error_code;

        if (priority_src->engines[0] == RI_AUTO) {
            // Do profiling to determine the priority
            error_code = ri_do_profiling(pattern, comp_options, exec_options,
                    priority_dst, log);
            if (error_code != RI_SUCCESS)
                return error_code;
        } else {
            // Use the setting of users
            *priority_dst = *priority_dst;
        }
    } else {
        // Use default setting
        *priority_dst = RI_PRIORITY_DEFAULT;
    }

    return RI_SUCCESS;
}

/**
** Check whether the priority is valid.
** @param priority:     The pointer to the priority.
** @param log:          The pointer to the log. It is optional.
**                      We would not output the log message if it is NULL.
** return: 0 if the priority is valid,
**         or the error code.
**/
static int ri_validate_priority(const struct ri_priority *priority, 
        const char **log)
{
    int error_code = 0;

    if (priority == NULL || priority->engine_count < 1) {
        error_code = RI_ERROR_REGEX_NULL;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    // If the first preference is RI_RE2, user must set the second priority,
    // which should be RI_PCRE or RI_PCRE-JIT.
    if (priority->engines[0] == RI_RE2) {
        if (!(priority->engine_count > 1 &&
              (priority->engines[1] == RI_PCRE || priority->engines[1] == RI_PCRE_JIT))) {
            error_code = RI_ERROR_INVALID_PRIORITY_RE2;
            ri_fill_log(log, RI_LOG_ERROR, error_code);
            return error_code;
        }
    }

    // To enable RI_AUTO, priority should be {RI_AUTO} and
    // priority_len = 1.
    if (priority->engines[0] == RI_AUTO && priority->engine_count != 1) {
        error_code = RI_ERROR_INVALID_PRIORITY_AUTO;
        ri_fill_log(log, RI_LOG_ERROR, error_code);
        return error_code;
    }

    // RI_AUTO can only appear on the first priority.
    for (int i = 1; i < priority->engine_count; i++) {
        if (priority->engines[i] == RI_AUTO) {
            error_code = RI_ERROR_INVALID_PRIORITY_AUTO;
            ri_fill_log(log, RI_LOG_ERROR, error_code);
            return error_code;
        }
    }

    return RI_SUCCESS;
}

/**
** Perform profiling to figure out the optimal priority of regex engines.
** @param pattern:      The pointer to the pattern.
** @param comp_options: Compilation options.
** @param exec_options: Execution options.
** @param priority:     The pointer to the priority output by profiling.
** @param log:          The pointer to the log. It is optional.
**                      We would not output the log message if it is NULL.
** return: 0 if the profiling succeeds,
**         or the error code.
**/
static int ri_do_profiling(const char *pattern, int comp_options, int exec_options,
        struct ri_priority *priority,  const char **log)
{
    // TODO: Determine the priority of engines based on profiling.
    return RI_ERROR_NOT_SUPPORT;
}

/**
** Get the detailed information of an error code.
** @param error_code: The code returned by a function.
** return: The string including the detailed information.
**/
const char *ri_get_error_msg(int error_code)
{
    switch (error_code) {
    case RI_SUCCESS:
        return "Success!";
    case RI_ERROR:
        return "Error!";
    case RI_ERROR_REGEX_NULL:
        return "ERROR: regex is NULL!";
    case RI_ERROR_PATTERN_NULL:
        return "ERROR: pattern is NULL!";
    case RI_ERROR_CANNOT_ALLOCATE_MEMORY_TO_REGEX:
        return "ERROR: Can NOT allocate memory to regex!";

    case RI_ERROR_PRIORITY_NULL:
        return "ERROR: The priority to update is NULL!";
    case RI_ERROR_INVALID_PRIORITY_LENGTH:
        return "ERROR: Invalid priority_len, which should be in \
            [0,SUPPORT_ENGINE_NUMBER]!";
    case RI_ERROR_INVALID_PRIORITY_RE2:
        return "ERROR: Invalid priority: the second priority should be \
            PCRE or PCRE-JIT if RE2 has the highest priority.";
    case RI_ERROR_INVALID_PRIORITY_AUTO:
        return "ERROR: priority should be {RI_AUTO} and priority_len is 1 \
            if RI_AUTO mode is preferred to use.";

    case RI_ERROR_PCRE_COMPILATION_FAILURE:
        return "ERROR: PCRE compilation fails.";
    case RI_ERROR_PCRE_NULL_POINTER:
        return "ERROR: PCRE pointer **re in compilation is NULL.";
    case RI_ERROR_PCRE_MALLOC:
        return "ERROR: pcre_malloc() fails.";
    case RI_ERROR_RE2_CANNOT_CONSTRUCT:
        return "ERROR: Can NOT construct RE2!";

    case RI_ERROR_EXEC_SUBJECT_NULL:
        return "ERROR: The subject to match is NULL!";
    case RI_ERROR_EXEC_INVALID_REGEX_ENGINE:
        return "ERROR: Invalid regex engine for execution.";
    case RI_ERROR_EXEC_NO_USABLE_REGEX_ENGINE:
        return "ERROR: No usable regex engine for execution.";
    
    case RI_ERROR_LOG_SPACE_INSUFFICIENT:
        return "ERROR: Log space is insufficient.";
    case RI_ERROR_LOG_FORMAT:
        return "ERROR: Log format error.";
    
    default:
        return "Error code is not found.";
    }
}

/**
** Fill in log according on the error_code
** @param log:        The pointer to the log.
** @param level:      The log level to fill the detailed error information.
** @param error_code: The error code.
** return: RI_SUCCESS if it succeeds, or the error code.
**/
int ri_fill_log(const char ** log, ri_log_level_t log_level, int error_code) {
    if (log != NULL && log_level <= g_ri_log_level) {
        *log = ri_get_error_msg(error_code);
    }
    return RI_SUCCESS;
}

/**
** Fill in log according on the format and rested paramenters
** @param log:        The pointer to the log.
** @param level:      The log level to fill the detailed error information.
** @param error_code: The error code.
** @param format:     The format string to record log, like printf.
** return: RI_SUCCESS if it succeeds, or the error code.
**/
int ri_fill_log_ex(const char ** log, ri_log_level_t log_level, const char * format, ...) {
    int size = 0;
    char *p = NULL;
    va_list ap;

    if (log == NULL || log_level > g_ri_log_level) {
        return RI_SUCCESS;
    }

    /* Determine required size */
    va_start(ap, format);
    size = vsnprintf(p, size, format, ap);
    va_end(ap);
    if (size < 0) {
        ri_fill_log(log, log_level, RI_ERROR_LOG_FORMAT);
        return RI_ERROR_LOG_FORMAT;
    }

    size++; /* For '\0' */
    if (size > g_ri_log_len) {
        if (g_ri_log_msg)
            free(g_ri_log_msg);
        g_ri_log_msg = NULL;
        g_ri_log_len = 0;
        p = malloc(size);
    } else {
        p = g_ri_log_msg;
    }
    if (p == NULL) {
        ri_fill_log(log, log_level, RI_ERROR_LOG_SPACE_INSUFFICIENT); 
        return RI_ERROR_LOG_SPACE_INSUFFICIENT;
    }

    va_start(ap, format);
    size = vsnprintf(p, size, format, ap);
    va_end(ap);

    if (size < 0)
    {
        free(p);
        ri_fill_log(log, log_level, RI_ERROR_LOG_FORMAT);
        return RI_ERROR_LOG_FORMAT;
    }

    g_ri_log_msg = p;
    g_ri_log_len = MAX(g_ri_log_len, size);

    *log = g_ri_log_msg;
    return RI_SUCCESS;
}

/**
** Perform initialization.
** It need be called at the first using of this module
**/
void ri_init()
{
    if (g_ri_log_msg != NULL)
        free(g_ri_log_msg);
    g_ri_log_msg = NULL;
    g_ri_log_len = 0;
}

/**
** Perform release.
** It need be called at the last using of this module
**/
void ri_release()
{
    if (g_ri_log_msg != NULL)
        free(g_ri_log_msg);
    g_ri_log_msg = NULL;
    g_ri_log_len = 0;
}

/**
** Get the global log level.
** return: The global log level.
**/
int ri_get_log_level()
{
    return g_ri_log_level;
}

/**
** Set the global log level.
** @param level: The level used to update log_level.
** return: 0 if update succeeds,
**         or the error code.
**/
int ri_set_log_level(ri_log_level_t level)
{
    g_ri_log_level = level;
    return RI_SUCCESS;
}
