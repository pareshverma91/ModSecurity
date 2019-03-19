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

#include <string>
#include <stdexcept>
#include <cwchar>
#include <cassert>

inline 
std::string ConvertWideCharToString(const wchar_t* source, unsigned int codepage = CP_ACP)
{
    assert(source != 0);
    const auto sourceLength = std::wcslen(source);
    if (sourceLength == 0) {
        return {};
    }

    const auto errorToException = [] {
        const auto error = GetLastError();
        switch (error) {
        case ERROR_INSUFFICIENT_BUFFER:
            throw std::out_of_range("Failed to convert wide char string: insufficient destination buffer length.");
        case ERROR_INVALID_FLAGS:
            throw std::logic_error("Invalid conversion flags supplied for wide char string.");
        case ERROR_INVALID_PARAMETER:
            throw std::invalid_argument("Invalid argument supplied for wide char string conversion.");
        case ERROR_NO_UNICODE_TRANSLATION:
            throw std::runtime_error("Invalid Unicode character found during wide char string conversion.");
        default:
            throw std::exception("Unknown error occurred during wide char string conversion.");
        }
    };

    const auto length = ::WideCharToMultiByte(codepage, 0, source, sourceLength, NULL, 0, NULL, NULL);
    if (length == 0) {
        errorToException();
    }
    
    // Allocate one extra byte for the terminating zero character
    std::string destination(length + 1, '\0');
    // C++11 standard (N3337, §21.4.5 [string.access]) guarantees the string storage to be contiguous
    // so it is safe to pass it as a destination buffer to the underlying conversion routine
    const auto result = ::WideCharToMultiByte(codepage, 0, source, sourceLength, &destination[0], length, NULL, NULL);
    if (result == 0) {
        errorToException();
    }

    return destination;
}
