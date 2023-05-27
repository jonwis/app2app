#pragma once

#include <winrt/App2App.h>
#include "dispatch_proxy_base.h"

namespace winrt
{
    struct caller_side_proxy : implements<caller_side_proxy, App2App::IApp2AppConnection>, caller_proxy_base
    {
        static App2App::IApp2AppConnection try_connect(winrt::guid const& id);

        Windows::Foundation::IAsyncOperation<App2App::App2AppCallResult> InvokeAsync(Windows::Foundation::Collections::IPropertySet args);
    };
}