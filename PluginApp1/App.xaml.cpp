// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"

#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include <winrt/App2App.h>
#include <winrt/Windows.Devices.Geolocation.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace PluginApp1;
using namespace PluginApp1::implementation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

struct MyGeoplugin : winrt::implements<MyGeoplugin, winrt::App2App::IApp2AppConnection>
{
    winrt::Windows::Foundation::IAsyncOperation<winrt::App2App::App2AppCallResult> InvokeAsync(winrt::Windows::Foundation::Collections::IPropertySet values)
    {
        auto lifetime{ get_strong() };

        co_await resume_background();

        winrt::Windows::Devices::Geolocation::Geolocator loc{};
        auto where = co_await loc.GetGeopositionAsync();
        auto coord = where.Coordinate();

        ValueSet vs{};
        vs.Insert(L"altitude", box_value(coord.Altitude()));
        vs.Insert(L"accuracy", box_value(coord.Accuracy()));
        vs.Insert(L"latitude", box_value(coord.Latitude()));
        vs.Insert(L"longitude", box_value(coord.Longitude()));

        co_return winrt::App2App::App2AppCallResult(winrt::App2App::App2AppCallResultStatus::Completed, S_OK, vs);
    }

    void Close()
    {
        m_closing(*this, nullptr);
    }

    auto Closed(TypedEventHandler<App2App::IApp2AppConnection, IInspectable> const& e)
    {
        return m_closing.add(e);
    }

    auto Closed(winrt::event_token const& t)
    {
        return m_closing.remove(t);
    }

    winrt::event<TypedEventHandler<App2App::IApp2AppConnection, IInspectable>> m_closing;
};

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();

    App2App::App2AppConnection::RegisterHost(winrt::guid{ "d69e1d12-c655-4378-80e1-48a9d649c35a" }, [] { return winrt::make< MyGeoplugin>(); });

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](IInspectable const&, UnhandledExceptionEventArgs const& e)
    {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

/// <summary>
/// Invoked when the application is launched.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs const&)
{
    window = make<MainWindow>();
    window.Activate();
}

