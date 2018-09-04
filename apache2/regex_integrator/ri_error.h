/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
#ifndef _RI_ERROR_H_
#define _RI_ERROR_H_

/**
** First Digit: equals 1 if it is an error code.
** Second Digit: the module where the error happens:
**               1 means in compilation, 2 means execution,
**               0 means internal error occurs besides compilatin and execution.
** Third and fourth Digit: the detailed error number.
**/
#define ERROR_TYPE int
#define ERROR_NO(NO) ((ERROR_TYPE)(-(NO)))

#define RI_SUCCESS ((ERROR_TYPE)(0x0000))

#define RI_ERROR    ERROR_NO(0x1000)

#define RI_ERROR_REGEX_NULL    ERROR_NO(0x1001)
#define RI_ERROR_PATTERN_NULL    ERROR_NO(0x1002)
#define RI_ERROR_CANNOT_ALLOCATE_MEMORY_TO_REGEX ERROR_NO(0x1003)
#define RI_ERROR_NOT_SUPPORT   ERROR_NO(0x1004)

#define RI_ERROR_PRIORITY_NULL ERROR_NO(0x1010)
#define RI_ERROR_INVALID_PRIORITY_LENGTH ERROR_NO(0x1011)
#define RI_ERROR_INVALID_PRIORITY_RE2 ERROR_NO(0x1012)
#define RI_ERROR_INVALID_PRIORITY_AUTO ERROR_NO(0x1013)

#define RI_ERROR_PCRE_COMPILATION_FAILURE ERROR_NO(0x1101)
#define RI_ERROR_PCRE_NULL_POINTER ERROR_NO(0x1102)
#define RI_ERROR_PCRE_MALLOC ERROR_NO(0x1103)
#define RI_ERROR_RE2_CANNOT_CONSTRUCT ERROR_NO(0x1104)

#define RI_ERROR_EXEC_SUBJECT_NULL    ERROR_NO(0x1200)
#define RI_ERROR_EXEC_INVALID_REGEX_ENGINE    ERROR_NO(0x1201)
#define RI_ERROR_EXEC_NO_USABLE_REGEX_ENGINE    ERROR_NO(0x1202)

#define RI_ERROR_LOG_SPACE_INSUFFICIENT ERROR_NO(0x1301)
#define RI_ERROR_LOG_FORMAT ERROR_NO(0x1302)

/**
** The log level.
** Use ri_get_log_level() to get the global log level.
** Use ri_set_log_level() to set the global log level.
**/
typedef enum {
    RI_LOG_NONE,
    RI_LOG_ERROR,
    RI_LOG_WARN,
    RI_LOG_INFO,
    RI_LOG_DEBUG
} ri_log_level_t;

#endif /* _RI_ERROR_H_ */
