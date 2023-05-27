#pragma once

#include <winrt/App2App.h>

namespace winrt
{
    struct caller_side_proxy : implements<caller_side_proxy, App2App::IApp2AppConnection>
    {
        static App2App::IApp2AppConnection try_connect(winrt::guid const& id);

        Windows::Foundation::IAsyncOperation<App2App::App2AppCallResult> InvokeAsync(Windows::Foundation::Collections::IPropertySet args);

        void Close();

        winrt::event_token Closed(Windows::Foundation::TypedEventHandler<App2App::IApp2AppConnection, Windows::Foundation::IInspectable> const& e);
        void Closed(event_token const& t);

    private:
        winrt::com_ptr<IDispatch> m_connection;
        DISPID m_callIds[2]{ -1,-1 };
        DISPID m_closeId{ -1 };
        event<Windows::Foundation::TypedEventHandler<App2App::IApp2AppConnection, Windows::Foundation::IInspectable>> m_closing;
    };
}