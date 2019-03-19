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

#include <memory>

//  IIS7 Server API header file
#include "httpserv.h"

//  Project header files
#include "string_conversion_utils.h"
#include "mymodule.h"
#include "mymodulefactory.h"
#include "moduleconfig.h"

static auto ReleaseAppHostProperty = [](IAppHostProperty* prop) {
    prop->Release();
};
using AppHostPropertyPtr = std::unique_ptr<IAppHostProperty, decltype(ReleaseAppHostProperty)>;

struct SafeVariant
{
    SafeVariant() {
        VariantInit(&var);
    }

    ~SafeVariant() {
        VariantClear(&var);
    }

    VARIANT var;
};

static bool GetBooleanProperty(IAppHostElement* element, wchar_t* propertyName)
{
    auto prop = [&] {
        IAppHostProperty* rawProperty = nullptr;
        HRESULT hr = element->GetPropertyByName(propertyName, &rawProperty);
        if (FAILED(hr)) {
            throw std::runtime_error("Couldn't get ModSecurity configuration property "
                + ConvertWideCharToString(propertyName)
                + ", error code " + std::to_string(hr));
        }
        if (!rawProperty) {
            throw std::runtime_error("Unexpected error while getting ModSecurity configuration property "
                + ConvertWideCharToString(propertyName));
        }
        return AppHostPropertyPtr{ rawProperty, ReleaseAppHostProperty };
    } ();

    SafeVariant var;
    HRESULT hr = prop->get_Value(&var.var);
    if (FAILED(hr)) {
        throw std::runtime_error("Couldn't extract value from property bag, error code " + std::to_string(hr));
    }

    IAppHostPropertyException* propertyException = nullptr;
    hr = prop->get_Exception(&propertyException);
    if (FAILED(hr)) {
        throw std::runtime_error("Got an exception while reading ModSecurity configuration property "
            + ConvertWideCharToString(propertyName)
            + ", error code " + std::to_string(hr));
    }
    if (propertyException) {
        throw std::runtime_error("Unexpected error while getting ModSecurity configuration property "
            + ConvertWideCharToString(propertyName));
    }

    return (var.var.boolVal == VARIANT_TRUE);
}

static std::wstring GetStringProperty(IAppHostElement* element, wchar_t* propertyName)
{
    auto prop = [&] {
        IAppHostProperty* rawProperty = nullptr;
        HRESULT hr = element->GetPropertyByName(propertyName, &rawProperty);
        if (FAILED(hr)) {
            throw std::runtime_error("Couldn't get ModSecurity configuration property "
                + ConvertWideCharToString(propertyName)
                + ", error code " + std::to_string(hr));
        }
        if (!rawProperty) {
            throw std::runtime_error("Unexpected error while getting ModSecurity configuration property "
                + ConvertWideCharToString(propertyName));
        }
        return AppHostPropertyPtr{ rawProperty, ReleaseAppHostProperty };
    } ();

    SafeVariant var;
    HRESULT hr = prop->get_Value(&var.var);
    if (FAILED(hr)) {
        throw std::runtime_error("Couldn't extract value from property bag, error code " + std::to_string(hr));
    }

    IAppHostPropertyException* propertyException = nullptr;
    hr = prop->get_Exception(&propertyException);
    if (FAILED(hr)) {
        throw std::runtime_error("Got an exception while reading ModSecurity configuration property "
            + ConvertWideCharToString(propertyName)
            + ", error code " + std::to_string(hr));
    }
    if (propertyException) {
        throw std::runtime_error("Unexpected error while getting ModSecurity configuration property "
            + ConvertWideCharToString(propertyName));
    }

    const auto length = SysStringLen(var.var.bstrVal);
    std::wstring result(var.var.bstrVal, length);

    return result;
}

ModSecurityStoredContext::ModSecurityStoredContext(IHttpContext* httpContext, ModSecurityStoredContext::ConstructorTag)
{
    std::wstring configSection {httpContext->GetMetadata()->GetMetaPath()};

    auto* adminManager = g_pHttpServer->GetAdminManager();
    if (!adminManager) {
        throw std::runtime_error("Unexpected error while getting admin manager.");
    }

    IAppHostElement* sessionTrackingElement = nullptr;
    HRESULT hr = adminManager->GetAdminSection(L"system.webServer/ModSecurity", &configSection[0], &sessionTrackingElement);
    if (FAILED(hr)) {
        throw std::runtime_error("Couldn't get ModSecurity configuration file section, error code " + std::to_string(hr));
    }
    if (!sessionTrackingElement) {
        throw std::runtime_error("Unexpected error while getting ModSecurity configuration file section.");
    }

    enabled = GetBooleanProperty(sessionTrackingElement, L"enabled");
    if (!enabled) {
        // No point in reading the rest of the config
        return;
    }

    configPath = GetStringProperty(sessionTrackingElement, L"configFile");
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
