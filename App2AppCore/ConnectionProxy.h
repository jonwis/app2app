#pragma once

#include <winrt/App2App.h>

namespace winrt
{
    struct connection_proxy : implements<connection_proxy, App2App::IApp2AppConnection>
    {
        static App2App::IApp2AppConnection try_connect(winrt::guid const& id);

        Windows::Foundation::IAsyncOperation<App2App::App2AppCallResult> InvokeAsync(Windows::Foundation::Collections::IPropertySet args);

        void Close();

        auto Closed(Windows::Foundation::TypedEventHandler<App2App::IApp2AppConnection, Windows::Foundation::IInspectable> const& e)
        {
            return m_closing.add(e);
        }

        auto Closed(event_token const& t)
        {
            return m_closing.remove(t);
        }

    private:
        App2App::IApp2AppConnection m_connection;
        event<Windows::Foundation::TypedEventHandler<App2App::IApp2AppConnection, Windows::Foundation::IInspectable>> m_closing;
    };
}