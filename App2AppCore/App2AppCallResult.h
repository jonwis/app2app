#pragma once
#include "App2AppCallResult.g.h"

namespace winrt::App2App::implementation
{
    struct App2AppCallResult : App2AppCallResultT<App2AppCallResult>
    {
        App2AppCallResult() = default;

        App2AppCallResult(winrt::hresult const& error, winrt::Windows::Foundation::IInspectable const& result);
        winrt::Windows::Foundation::IInspectable Result();
    };
}
namespace winrt::App2App::factory_implementation
{
    struct App2AppCallResult : App2AppCallResultT<App2AppCallResult, implementation::App2AppCallResult>
    {
    };
}
