#pragma once

#if defined(_WIN32)

// Forward declare winrt::hresult to avoid including heavy headers here.
// The actual definition will be pulled in by the source files that use it.
namespace winrt {
    struct hresult;
}

#include <QtCore/QString>

// Specialize QtPrivate::treat_as_integral_arg for winrt::hresult.
// This informs QString::arg that this type should be treated as an integral type,
// allowing it to be formatted as a number (which it is, implicitly convertible to int32_t).
// This resolves the "no matching overloaded function" error in Qt 6.10+.
namespace QtPrivate {
    template <>
    struct treat_as_integral_arg<winrt::hresult> : std::true_type {};
}

#endif
