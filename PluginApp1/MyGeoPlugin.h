#pragma once

#include <winrt/App2App.h>

/*
* This is the definition for a simple plugin that reports the current geoposition of the user's
* machine. It implements the IApp2AppConnection interface in the usual way - see MyGeoPlugin.cpp
* for details.
*/
struct MyGeoplugin : winrt::implements<MyGeoplugin, winrt::App2App::IApp2AppConnection>
{
    winrt::Windows::Foundation::IAsyncOperation<winrt::App2App::App2AppCallResult> InvokeAsync(winrt::Windows::Foundation::Collections::IPropertySet values);

    void Close();

    winrt::event_token Closed(winrt::Windows::Foundation::TypedEventHandler<winrt::App2App::IApp2AppConnection, IInspectable> const& e);
    void Closed(winrt::event_token const& t);

    winrt::event<winrt::Windows::Foundation::TypedEventHandler<winrt::App2App::IApp2AppConnection, winrt::Windows::Foundation::IInspectable>> m_closing;

    static void Register();
};
