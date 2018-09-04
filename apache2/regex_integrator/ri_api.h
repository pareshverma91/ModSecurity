/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
#ifndef _RI_API_H_
#define _RI_API_H_

#include "ri_error.h"

typedef void *ri_regex_t;

typedef enum
{
    RI_PCRE,         // Use PCRE
    RI_PCRE_JIT,     // Use PCRE-JIT
    RI_RE2,          // Use RE2
    RI_AUTO,         // Select regex engine automatically
    RI_ENGINE_COUNT, // The supported count of regex integrator
} ri_engine_t;

/* Options for compilation. */
enum ri_comp_option
{
    RI_COMP_DEFAULT = 0x000000,        // Default Option
    RI_COMP_CASELESS = 0x000001,       // PCRE_CASELESS
    RI_COMP_MULTILINE = 0x000002,      // PCRE_MULTILINE
    RI_COMP_DOTALL = 0x000004,         // PCRE_DOTALL
    RI_COMP_DOLLAR_ENDONLY = 0x000008, // PCRE_DOLLAR_ENDONLY
};

/* Options for execution. */
enum ri_exec_option
{
    RI_EXEC_DEFAULT = 0x000000,  // Default Option
    RI_EXEC_NOTEMPTY = 0x001000, // PCRE_NOTEMPTY
};

struct ri_priority
{
    ri_engine_t engines[RI_ENGINE_COUNT];
    unsigned int engine_count; // The count of engine
};

struct ri_pcre_params
{
    int match_limit;
    int match_limit_recursion;
};

struct ri_params
{
    struct ri_pcre_params pcre;
    struct ri_pcre_params pcre_jit;
};

#ifdef __cplusplus
extern "C"
{
#endif

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
              const char **log);

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
            const struct ri_priority *priority, int *ovector, int ovec_size,
            const char **log);

/**
** Free the compiled regex.
** @param regex: The compiled regex.
**/
void ri_free(ri_regex_t regex);

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
                    const struct ri_priority *priority_new,
                    const char **log);

/**
** Get the detailed information of an error code.
** @param error_code: The code returned by a function.
** return: The string including the detailed information.
**/
const char *ri_get_error_msg(int error_code);

/**
** Perform initialization.
** It need be called at the first using of this module
**/
void ri_init();

/**
** Perform release.
** It need be called at the last using of this module
**/
void ri_release();

/**
** Get the global log level.
** return: The global log level.
**/
int ri_get_log_level();

/**
** Set the global log level.
** @param level: The level used to update log_level.
** return: 0 if update succeeds,
**         or the error code.
**/
int ri_set_log_level(ri_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* _RI_API_H_ */
