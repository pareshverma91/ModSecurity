#ifndef _WAF_LOG_UTIL_EXTERNAL_HEADER
#define _WAF_LOG_UTIL_EXTERNAL_HEADER

#define WAF_LOG_UTIL_FAILED -1
#define WAF_LOG_UTIL_SUCCESS 0
#define WAF_RULESET_PREFIX "/RuleSets/"
#ifdef __cplusplus
extern "C" {
#endif
int write_json_to_file(char* data_dir, char* clientIP, char* clientPort, char* requestUri, char* ruleSetType, char* ruleSetVersion, char* ruleId, char* messages, int action, int site, char* details_messages, char* details_data, char* details_file, char* details_line, char* hostname, char* time);
#ifdef __cplusplus
}
#endif

#endif
