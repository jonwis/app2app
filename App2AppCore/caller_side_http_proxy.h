#pragma once

#include <winrt/App2App.h>

namespace winrt
{
    struct caller_side_http_proxy : implements<caller_side_http_proxy, App2App::IApp2AppHttpConnection>
    {
        static App2App::IApp2AppHttpConnection try_connect(winrt::guid const& id);

        Windows::Foundation::IAsyncOperation<Windows::Web::Http::HttpResponseMessage> InvokeAsync(Windows::Web::Http::HttpRequestMessage args);

        void Close();

    private:
        winrt::com_ptr<IDispatch> m_connection;
        DISPID m_callHttpIds[2]{ -1,-1 };
        DISPID m_closeId{ -1 };
    };
}