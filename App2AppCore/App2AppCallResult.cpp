#include "pch.h"
#include "App2AppCallResult.h"
#include "App2AppCallResult.g.cpp"

namespace winrt::App2App::implementation
{
    App2AppCallResult::App2AppCallResult(winrt::hresult const& error, winrt::Windows::Foundation::IInspectable const& result)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IInspectable App2AppCallResult::Result()
    {
        throw hresult_not_implemented();
    }
}
