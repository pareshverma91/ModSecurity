#ifndef _WAF_LOG_UTIL_INTERNAL_HEADER
#define _WAF_LOG_UTIL_INTERNAL_HEADER

#include <fstream>
#include <unordered_map>
#include <exception>
#include "../../jsoncons/include/jsoncons/json.hpp"
#include "../../jsoncons/include/jsoncons/json_options.hpp"

// For convenience
using jsoncons::ojson;
using jsoncons::json_options;
using jsoncons::line_split_kind;

namespace ns {

    struct detail
    {
        std::string message;
        std::string data;
        std::string file;
        std::string line;
    };

    struct property
    {
        std::string instanceId;
        std::string clientIp;
        std::string clientPort;
        std::string requestUri;
        std::string ruleSetType;
        std::string ruleSetVersion;
        std::string ruleId;
        std::string message;
        std::string action;
        std::string site;
        detail details;
        std::string hostname;
        std::string transactionId;
    };

    class waf_log_object
    {
        std::string resourceId;
        std::string operationName;
        std::string category;
        property properties;

        // Make json_type_traits specializations friends to give accesses to private members
        JSONCONS_TYPE_TRAITS_FRIEND;
    public:
        waf_log_object(){
        }

        waf_log_object(const std::string& resourceId, const std::string& operationName, const std::string& category, const property& properties)
            : resourceId(resourceId), operationName(operationName), category(category), properties(properties) {                
        }
        
        std::string to_json_string(){
            ojson detail;
            ojson property;
            ojson waf_log;

            json_options options;
            options.indent(0);
            options.new_line_chars(" ");
            std::stringstream buffer;
            
            property["instanceId"] = properties.instanceId;
            property["clientIp"] = properties.clientIp;                
            property["clientPort"] = properties.clientPort;                
            property["requestUri"] = properties.requestUri;                
            property["ruleSetType"] = properties.ruleSetType;                
            property["ruleSetVersion"] = properties.ruleSetVersion;                
            property["ruleId"] = properties.ruleId;                
            property["message"] = properties.message;                
            property["action"] = properties.action;                
            property["site"] = properties.site;

            detail["message"] = properties.details.message;
            detail["data"] = properties.details.data;
            detail["file"] = properties.details.file;
            detail["line"] = properties.details.line;

            property["details"] = std::move(detail); 
            property["hostname"] = properties.hostname;
            property["transactionId"] = properties.transactionId;

            waf_log["resourceId"] = resourceId; 
            waf_log["operationName"] = operationName; 
            waf_log["category"] = category;             
            waf_log["properties"] = std::move(property);

            buffer << pretty_print(waf_log, options);
            return buffer.str();
        }
    };
} // namespace ns

#endif 
