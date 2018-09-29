// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>

#include <re2/re2.h>
#include <pcre.h>
#include "ri_api.h"

#define OVECCOUNT 1024

#define BENCH_TIME 3 // seconds
#define BENCH(STATEMENT, OPS) do{                               \
        struct timeb _start = {0}, _end = {0};                  \
        int _time = 0, _i = 0;                                  \
        int _eslapse =0;                                        \
        ftime(&_start);                                         \
        do {                                                    \
            for (_i = 0; _i < 10; ++_i) {                       \
                STATEMENT;                                      \
                ++_time;                                        \
            }                                                   \
            ftime(&_end);                                       \
            _eslapse = (1000 * (_end.time - _start.time)        \
                + (_end.millitm - _start.millitm));             \
        } while(_eslapse < (BENCH_TIME * 1000));                \
        OPS = (((double)_time / _eslapse) * 1000);              \
    }while(0);
    

struct ri_priority g_default_ri_priority = {
    {RI_PCRE, RI_PCRE_JIT, RI_PCRE},
    3,
};

const char * human_readable(double number) {
    static char buffer[1024] = {0};
    char unit = ' ';

    if (number >= 1e9) {
        unit = 'G';
        number /= 1e9;
    } else if (number > 1e6) {
        unit = 'M';
        number /= 1e6;
    } else if (number > 1e3) {
        unit = 'K';
        number /= 1e3;
    }
    sprintf(buffer, "%.3f %c", number, unit);
    return buffer;
}

int test(
    const std::string & pattern_str, 
    const std::string & subject_str, 
    int comp_opt,   // Options for compilation
    int exec_opt,   // Options for execution
    int exec_expect // Expected result for execution
    ) {
    int ret = 0;
    double ops = 0;

    //print limiter
    for (int i =0; i < 120;i++) {
        fprintf(stderr,"-");
    }
    fprintf(stderr, "\n");
 
    fprintf(stderr, 
            "BENCH START\n\t"                      
            "pattern(%s)\n\t"                    
            "subject(%s)\n\t"                    
            "compile option(%d)\n\t"              
            "execution option(%d)\n\t"
            "expected execution result(%d)\n", 
            pattern_str.c_str(), 
            subject_str.c_str(),
            comp_opt, 
            exec_opt,
            exec_expect);


    // Regex Integrator

    {
        ri_regex_t ri_regex = NULL;
        const char *log = NULL;
        int ovector[OVECCOUNT] = {0};

        ri_set_log_level(RI_LOG_ERROR);

        ret = ri_create(&ri_regex, pattern_str.c_str(), comp_opt, &g_default_ri_priority, NULL, &log);
        if (ret != RI_SUCCESS)
        {
            fprintf(stderr, "RI compilation fail: %s\n", ri_get_error_msg(ret));
            goto ri_finish;
        }

        BENCH(
            ret = ri_exec(ri_regex, subject_str.c_str(), subject_str.length(), 0, exec_opt,NULL , ovector, OVECCOUNT, NULL);
            if (ret != exec_expect) {
                fprintf(stderr, "RI match result is unmatched.\n");
                fprintf(stderr, "%s\n", ri_get_error_msg(ret));
                goto ri_finish;
            },
            ops);

        if (ret != exec_expect)
        {
            fprintf(stderr, "Unexpected execution result: expect(%d) get(%d)\n", exec_expect, ret);
            goto ri_finish;
        }
        fprintf(stderr, "RI OPS(%s)\n", human_readable(ops));
        // fprintf(stderr, "OPS(%.3f)\n", ops);

    ri_finish:
        ri_free(ri_regex);
        ri_regex = NULL;
        fprintf(stderr, "RI BENCH FINISH\n");
    }

    // RE2
    {
        std::string re2_pattern = pattern_str;
        RE2::Options re2_options;
        re2_options.set_encoding(RE2::Options::EncodingLatin1);

        // We store the error message in the log instead of printing it out
        re2_options.set_log_errors(false);

        if (comp_opt & RI_COMP_CASELESS)
        {
            re2_options.set_case_sensitive(false);
        }

        if (comp_opt & RI_COMP_DOTALL)
        {
            re2_options.set_dot_nl(true);
        }

        // To enable MULTILINE options, modify the pattern to (?m:pattern)
        if (comp_opt & RI_COMP_MULTILINE)
        {
            re2_pattern = "(?m:" + re2_pattern + ")";
        }
        re2::RE2 re2_regex(re2_pattern, re2_options);
        re2::StringPiece submatches[1 + re2_regex.NumberOfCapturingGroups()];
        if (!re2_regex.ok())
        {
            fprintf(stderr, "RE2 compilation fail: %s\n", re2_regex.error().c_str());
            goto re2_finish;
        }

        BENCH({
            if (
                re2_regex.Match(
                    subject_str.c_str(), 
                    0, 
                    subject_str.length(), 
                    RE2::UNANCHORED, 
                    submatches, 
                    sizeof(submatches)/sizeof(submatches[0]))) {
                if (exec_expect <= 0) {
                    fprintf(stderr, "RE2 match result is unmatched.\n");
                    goto re2_finish;
                }
            } else {
                if (exec_expect > 0) {
                    fprintf(stderr, "RE2 match result is unmatched.\n");
                    goto re2_finish;
                }
            } },
              ops);
        fprintf(stderr, "RE2 OPS(%s)\n", human_readable(ops));
    re2_finish:
        fprintf(stderr, "RE2 BENCH FINISH\n");
    }

    //PCRE
    {
        int pcre_options = 0;
        const char * error_ptr;
        int error_offset;
        int rt;
        int ovector[OVECCOUNT] = {0};

        if (comp_opt & RI_COMP_CASELESS)
            pcre_options |= PCRE_CASELESS;

        if (comp_opt & RI_COMP_MULTILINE)
            pcre_options |= PCRE_MULTILINE;

        if (comp_opt & RI_COMP_DOTALL)
            pcre_options |= PCRE_DOTALL;

        if (comp_opt & RI_COMP_DOLLAR_ENDONLY)
            pcre_options |= PCRE_DOLLAR_ENDONLY;

        void * pcre_regex = pcre_compile(pattern_str.c_str(), pcre_options, &error_ptr, &error_offset, NULL);
        void * pcre_pe = NULL;
        if (pcre_regex != NULL) {
            pcre_pe = pcre_study((pcre *)pcre_regex, 0, &error_ptr);
        }
        if ( pcre_regex ==NULL ) {
            fprintf(stderr, "PCRE compilation fail : %s, offset = %d\n", error_ptr, error_offset);
            goto pcre_finish;
        }
        pcre_options = 0;
        if (exec_opt & RI_EXEC_NOTEMPTY)
            pcre_options |= PCRE_NOTEMPTY;

        BENCH({
            rt = pcre_exec(
                (pcre *)pcre_regex, 
                (pcre_extra *)pcre_pe, 
                subject_str.c_str(), 
                subject_str.length(), 
                0, 
                pcre_options, 
                ovector, 
                OVECCOUNT);
            if (rt != exec_expect) {
                fprintf(stderr, "PCRE match result is unmatched.\n");
                goto pcre_finish;
            } }
            ,ops
        );
        fprintf(stderr, "PCRE OPS(%s)\n", human_readable(ops));
    pcre_finish:;
        fprintf(stderr, "PCRE BENCH FINISH \n");
        if (pcre_regex) {
            pcre_free(pcre_regex);
        }
    }

    //PCRE-JIT
    {
        int pcre_options = 0;
        const char * error_ptr;
        int error_offset;
        int rt;
        int ovector[OVECCOUNT] = {0};

        if (comp_opt & RI_COMP_CASELESS)
            pcre_options |= PCRE_CASELESS;

        if (comp_opt & RI_COMP_MULTILINE)
            pcre_options |= PCRE_MULTILINE;

        if (comp_opt & RI_COMP_DOTALL)
            pcre_options |= PCRE_DOTALL;

        if (comp_opt & RI_COMP_DOLLAR_ENDONLY)
            pcre_options |= PCRE_DOLLAR_ENDONLY;

        void * pcre_regex = pcre_compile(pattern_str.c_str(), pcre_options, &error_ptr, &error_offset, NULL);
        void * pcre_pe = NULL;
        if (pcre_regex != NULL) {
            pcre_pe = pcre_study((pcre *)pcre_regex, PCRE_STUDY_JIT_COMPILE, &error_ptr);
        }
        if (pcre_regex == NULL || pcre_pe == NULL){
            fprintf(stderr, "PCRE-JIT compile fail : %s, offset = %d\n", error_ptr, error_offset);
            goto pcre_jit_finish;
        }

        pcre_options = 0;
        if (exec_opt & RI_EXEC_NOTEMPTY)
            pcre_options |= PCRE_NOTEMPTY;

        BENCH({
            rt = pcre_exec(
                (pcre *)pcre_regex, 
                (pcre_extra *)pcre_pe, 
                subject_str.c_str(), 
                subject_str.length(), 
                0, 
                pcre_options, 
                ovector, 
                OVECCOUNT);
            if (rt != exec_expect) {
                fprintf(stderr, "PCRE-JIT match result is unmatched.\n");
                goto pcre_jit_finish;
            } }
            ,ops
        );
        fprintf(stderr, "PCRE-JIT OPS(%s)\n", human_readable(ops));
    pcre_jit_finish:;
        fprintf(stderr, "PCRE-JIT BENCH FINISH \n");
        if (pcre_regex) {
            pcre_free(pcre_regex);
        }
        if (pcre_pe) {
            pcre_free_study((pcre_extra *)pcre_pe);
        }
    }

    return 0;

}

int main(int argc, char *argv[]) {
    ri_init();

    if(argc == 4) {
        if (test(argv[1], argv[2], RI_COMP_DEFAULT, RI_EXEC_DEFAULT, atoi(argv[3])) != 0) {
            goto exit_error;
        }
        ri_release();
        return 0;
    }

    // Normal match
    if (test("abc", "abc", RI_COMP_DEFAULT, RI_EXEC_DEFAULT, 1) != 0) {
        goto exit_error;
    }

    // Normal un-match
    if (test("def", "abc", RI_COMP_DEFAULT, RI_EXEC_DEFAULT, -1) != 0) {
        goto exit_error;
    }

    // Compile error
    if (test("{abc(:", "abc", RI_COMP_DEFAULT, RI_EXEC_DEFAULT, -1) != 0) {
        goto exit_error;
    }

    // Large match
    if (test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 
        RI_COMP_DEFAULT, 
        RI_EXEC_DEFAULT,
        1) != 0) {
        goto exit_error;
    }

    // A challenging case for PCRE
    if (test("a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?aaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 
        RI_COMP_DEFAULT, 
        RI_EXEC_DEFAULT,
        1) != 0) {
        goto exit_error;
    }

    // Multi-line support
    if (test("b$", "b\nb", RI_COMP_MULTILINE, RI_EXEC_DEFAULT, 1) != 0) {
        goto exit_error;
    }

    // Caseless support
    if (test("ABC", "abc", RI_COMP_CASELESS, RI_EXEC_DEFAULT, 1) != 0) {
        goto exit_error;
    }

    // Capture groups
    if (test("(a+b)(c+d)(e+f)", 
        "abcdef", 
        RI_COMP_DEFAULT, 
        RI_EXEC_DEFAULT,
        4) != 0) {
        goto exit_error;
    }

    // No enough space to store capture groups
    if (test("(a?)(b?)(c?)(d?)(e?)(f?)(g?)(h?)(i?)(j?)(k?)", 
        "abcdefghijk", 
        RI_COMP_DEFAULT, 
        RI_EXEC_DEFAULT,
        0) != 0) {
        goto exit_error;
    }    

    // Not empty match
    if (test("a?b?c?d?e?", 
        "ffff", 
        RI_COMP_DEFAULT, 
        RI_EXEC_NOTEMPTY,
        -1) != 0) {
        goto exit_error;
    }       

    ri_release();
    return 0;

exit_error:
    ri_release();
    return -1;
}
