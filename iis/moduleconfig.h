/*
* ModSecurity for Apache 2.x, http://www.modsecurity.org/
* Copyright (c) 2004-2013 Trustwave Holdings, Inc. (http://www.trustwave.com/)
*
* You may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* If any of the files related to licensing are missing or if you have any
* other questions related to licensing please contact Trustwave Holdings, Inc.
* directly using the email address security@modsecurity.org.
*/

#pragma once

extern IHttpServer *                       g_pHttpServer;

extern PVOID                               g_pModuleContext;

struct directory_config; // forward declaration

class ModSecurityStoredContext
    : public IHttpStoredContext
{
private:
    bool enabled = false;
    directory_config* config = nullptr;

    // Needed to enable std::make_unique inside the factory static method.
    // See https://seanmiddleditch.com/enabling-make_unique-with-private-constructors/ for details.
    struct ConstructorTag { 
        explicit ConstructorTag() = default; 
    };

public:
    static ModSecurityStoredContext* GetConfiguration(IHttpContext* httpContext);

    explicit ModSecurityStoredContext(IHttpContext* httpContext, ConstructorTag);

    void CleanupStoredContext() override {
        delete this;
    }

    bool IsEnabled() const {
        return enabled;
    }

    directory_config* GetDirectoryConfig() const {
        return config;
    }  
};
