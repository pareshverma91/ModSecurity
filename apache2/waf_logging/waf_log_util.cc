#include "waf_log_util_internal.h"
#include "waf_log_util_external.h"

// This function fills in a waf format message based on modsec input.
void set_waf_format(waf_format::Waf_Format* waf_format, char* clientIP, char* clientPort, char* requestUri, char* ruleSetType, char* ruleSetVersion, char* ruleId, char* messages, int action, int site, char* details_messages, char* details_data, char* details_file, char* details_line, char* hostname) {
  if (clientIP != NULL) {
      waf_format->set_clientip(clientIP);
  }

  if (clientPort != NULL) {
      waf_format->set_clientport(clientPort);
  }

  if (requestUri != NULL) {
      waf_format->set_requesturi(requestUri);
  }

  if (ruleSetType != NULL) {
      waf_format->set_rulesettype(ruleSetType);
  }

  if (ruleSetVersion != NULL) {
      waf_format->set_rulesetversion(ruleSetVersion);
  }

  if (ruleId != NULL) {
      waf_format->set_ruleid(ruleId);
  }

  if (messages != NULL) {
      waf_format->set_messages(messages);
  }

  switch(action) {
      case 1:
          waf_format->set_action(waf_format::Waf_Format::Detected);
      case 2:
          waf_format->set_action(waf_format::Waf_Format::Blocked);
  }

  if (site == 0) {
      waf_format->set_site(waf_format::Waf_Format::Global);
  }

  waf_format::Details details;

  if (details_messages != NULL) {
      details.set_messages(details_messages);
  }

  if (details_data != NULL) {
      details.set_data(details_data);
  }

  if (details_file != NULL) {
      details.set_file(details_file);
  }

  if (details_line != NULL) {
      details.set_line(details_line);
  }

  *waf_format->mutable_details() = details;

  if (hostname != NULL) {
      waf_format->set_hostname(hostname);
  }
}

// Main function:  get fields from modsec, set the protobuf object and write to file in json.
int write_json_to_file(char* clientIP, char* clientPort, char* requestUri, char* ruleSetType, char* ruleSetVersion, char* ruleId, char* messages, int action, int site, char* details_messages, char* details_data, char* details_file, char* details_line, char* hostname) {
  waf_format::Waf_Format waf_format;
  std::ofstream json_file;
  std::string json_string;
  google::protobuf::util::JsonPrintOptions options;
  google::protobuf::util::Status convert_result;

  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  try {
      GOOGLE_PROTOBUF_VERIFY_VERSION;
  }
  catch (std::exception& e) {
      return WAF_LOG_UTIL_FAILED;
  }

  json_file.open(WAF_LOG_UTIL_PATH, std::ios::app);
  if (json_file.fail())
  {
     return WAF_LOG_UTIL_FAILED;
  }

  // Set Waf format.
  try {
      set_waf_format(&waf_format, clientIP, clientPort, requestUri, ruleSetType, ruleSetVersion, ruleId, messages, action, site, details_messages, details_data, details_file, details_line, hostname); 
  }
  catch (std::exception& e) {
      return WAF_LOG_UTIL_FAILED;
  }

  options.add_whitespace = true;
  options.always_print_primitive_fields = true;
  options.preserve_proto_field_names = true;

  try {
      convert_result = MessageToJsonString(waf_format, &json_string, options);
  }
  catch (std::exception& e) {
      return WAF_LOG_UTIL_FAILED;
  }

  if (!convert_result.ok()) {
      return WAF_LOG_UTIL_FAILED;
  }

    // Write the new address book back to disk.
  json_file << json_string << std::endl;

  json_file.close();
  // Optional:  Delete all global objects allocated by libprotobuf.
  //google::protobuf::ShutdownProtobufLibrary();

  return WAF_LOG_UTIL_SUCCESS;
}
