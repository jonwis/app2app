#pragma once

#include <winrt/App2App.h>

struct MyRestWebPlugin : winrt::implements<MyRestWebPlugin, winrt::App2App::IApp2AppHttpConnection>
{
    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Web::Http::HttpResponseMessage> InvokeAsync(winrt::Windows::Web::Http::HttpRequestMessage values);

    static void Register();
};
