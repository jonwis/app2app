#pragma once
#include "App2AppConnection.g.h"

namespace winrt::App2App::implementation
{
    struct App2AppConnection
    {
        App2AppConnection() = default;

        static winrt::App2App::IApp2AppConnection Connect(hstring const& packageFamilyName, hstring const& service);
    };
}
namespace winrt::App2App::factory_implementation
{
    struct App2AppConnection : App2AppConnectionT<App2AppConnection, implementation::App2AppConnection>
    {
    };
}
