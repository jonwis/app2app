#pragma once
#include "App2AppConnection.g.h"
#include <mutex>

namespace winrt::App2App::implementation
{
    struct App2AppConnection
    {
        App2AppConnection() = default;

        static winrt::App2App::IApp2AppConnection Connect(hstring const& packageFamilyName, hstring const& service);
        static winrt::com_array<winrt::Windows::ApplicationModel::Package> GetPackagesWithService(hstring const& service);
        static winrt::App2App::IApp2AppConnection ConnectToService(hstring const& service);

        static void RegisterHost(winrt::guid const& hostId, RequestConnectionHostDelegate delegate);
        static void DeregisterHost(winrt::guid const& hostId);
        static void UnregisterAllHosts();
    };
}
namespace winrt::App2App::factory_implementation
{
    struct App2AppConnection : App2AppConnectionT<App2AppConnection, implementation::App2AppConnection>
    {
    };
}
