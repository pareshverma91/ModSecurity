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

#define WIN32_LEAN_AND_MEAN

#undef inline

// C++ Standard Library smart pointers
#include <memory>

// For _variant_t wrapper
#include <comdef.h>

//  IIS7 Server API header file
#include "httpserv.h"

//  Project header files
#include "string_conversion_utils.h"
#include "mymodule.h"
#include "mymodulefactory.h"
#include "moduleconfig.h"

template <class T>
struct InterfaceReleaser
{
    void operator() (T* interfacePtr) const {
        interfacePtr->Release();
    }
};

template <class T>
using InterfaceUniquePtr = std::unique_ptr<T, InterfaceReleaser<T>>;

using AppHostElementPtr = InterfaceUniquePtr<IAppHostElement>;
using AppHostPropertyPtr = InterfaceUniquePtr<IAppHostProperty>;

static AppHostElementPtr GetConfigElement(IAppHostAdminManager* adminManager, IHttpContext* httpContext)
{
    std::wstring configSection {httpContext->GetMetadata()->GetMetaPath()};
    IAppHostElement* configElement = nullptr;
    HRESULT hr = adminManager->GetAdminSection(L"system.webServer/ModSecurity", &configSection[0], &configElement);
    if (FAILED(hr)) {
        throw std::runtime_error("Couldn't get ModSecurity configuration file section, error code " + std::to_string(hr));
    }
    if (!configElement) {
        throw std::runtime_error("Unexpected error while getting ModSecurity configuration file section.");
    }

    return {configElement, {}};
}

static AppHostPropertyPtr GetPropertyPtr(IAppHostElement* configElement, wchar_t* name)
{
    IAppHostProperty* prop = nullptr;
    HRESULT hr = configElement->GetPropertyByName(name, &prop);
    if (FAILED(hr)) {
        throw std::runtime_error("Couldn't get ModSecurity configuration property "
            + ConvertWideCharToString(name)
            + ", error code " + std::to_string(hr));
    }
    if (!prop) {
        throw std::runtime_error("Unexpected error while getting ModSecurity configuration property "
            + ConvertWideCharToString(name));
    }
    return {prop, {}};
}

static _variant_t GetProperty(IAppHostElement* configElement, wchar_t* name)
{
    auto prop = GetPropertyPtr(configElement, name);
    _variant_t var;
    HRESULT hr = prop->get_Value(&var);
    if (FAILED(hr)) {
        throw std::runtime_error("Couldn't extract value from property bag, error code " + std::to_string(hr));
    }

    IAppHostPropertyException* propertyException = nullptr;
    hr = prop->get_Exception(&propertyException);
    if (FAILED(hr)) {
        throw std::runtime_error("Got an exception while reading ModSecurity configuration property "
            + ConvertWideCharToString(name)
            + ", error code " + std::to_string(hr));
    }
    if (propertyException) {
        throw std::runtime_error("Unexpected error while getting ModSecurity configuration property "
            + ConvertWideCharToString(name));
    }

    return var;
}

static bool GetBooleanProperty(IAppHostElement* element, wchar_t* name)
{
    const auto var = GetProperty(element, name);
    return var.boolVal == VARIANT_TRUE;
}

static std::wstring GetStringProperty(IAppHostElement* element, wchar_t* name)
{
    const auto var = GetProperty(element, name);

    const auto length = SysStringLen(var.bstrVal);
    return std::wstring(var.bstrVal, length);
}

ModSecurityStoredContext::ModSecurityStoredContext(IHttpContext* httpContext, ModSecurityStoredContext::ConstructorTag)
{
    auto configElement = GetConfigElement(g_pHttpServer->GetAdminManager(), httpContext);
    enabled = GetBooleanProperty(configElement.get(), L"enabled");
    if (!enabled) {
        // No point in reading the rest of the config
        return;
    }

    configPath = GetStringProperty(configElement.get(), L"configFile");
}

ModSecurityStoredContext* ModSecurityStoredContext::GetConfiguration(IHttpContext* httpContext)
{
    IHttpModuleContextContainer* metadataContainer = httpContext->GetMetadata()->GetModuleContextContainer();
    if (!metadataContainer) {
        throw std::runtime_error("Couldn't get metadata container.");
    }

    ModSecurityStoredContext* context = (ModSecurityStoredContext*)metadataContainer->GetModuleContext(g_pModuleContext);
    if (context) {
        // We found stored data for this module for the metadata
        // object which is different for unique configuration path
        return context;
    }

    // If we reach here, that means this is first request or first
    // request after a configuration change IIS core will throw stored context away
    // if a change notification arrives for this metadata path
    auto newContext = std::make_unique<ModSecurityStoredContext>(httpContext, ConstructorTag{});

    // Store ModSecurityStoredContext data as metadata stored context
    HRESULT hr = metadataContainer->SetModuleContext(newContext.get(), g_pModuleContext);
    if (FAILED(hr)) {
        // It is possible that some other thread stored context before this thread
        // could do. Check returned value and return context stored by other thread
        if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_ASSIGNED)) {
            return (ModSecurityStoredContext*)metadataContainer->GetModuleContext(g_pModuleContext);
        }

        throw std::runtime_error("Couldn't store module configuration, error code: " + std::to_string(hr));
    }

    return newContext.release();
}
