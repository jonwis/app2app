#pragma once

#include <winrt/App2App.h>
#include <dispatch_proxy_base.h>

namespace winrt
{
    struct caller_side_http_proxy : implements<caller_side_http_proxy, App2App::IApp2AppHttpConnection>, caller_proxy_base
    {
        static App2App::IApp2AppHttpConnection try_connect(winrt::guid const& id);

        Windows::Foundation::IAsyncOperation<Windows::Web::Http::HttpResponseMessage> InvokeAsync(Windows::Web::Http::HttpRequestMessage args);
    };
}