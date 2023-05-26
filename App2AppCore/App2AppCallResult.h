#pragma once
#include "App2AppCallResult.g.h"

namespace winrt::App2App::implementation
{
    struct App2AppCallResult : App2AppCallResultT<App2AppCallResult>
    {
        App2AppCallResult(App2AppCallResultStatus status, winrt::hresult const& error, winrt::Windows::Foundation::IInspectable const& result) :
            m_status(status),
            m_error(error),
            m_result(result)
        {
        }

        winrt::Windows::Foundation::IInspectable Result()
        {
            return m_result;
        }

        App2AppCallResultStatus Status()
        {
            return m_status;
        }

        winrt::hresult ExtendedError()
        {
            return m_error;
        }

        App2AppCallResultStatus m_status;
        winrt::hresult m_error;
        winrt::Windows::Foundation::IInspectable m_result;
    };
}
namespace winrt::App2App::factory_implementation
{
    struct App2AppCallResult : App2AppCallResultT<App2AppCallResult, implementation::App2AppCallResult>
    {
    };
}
