// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/App2App.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Web::Http;
using namespace App2App;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::PluginCaller2::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    winrt::fire_and_forget MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto lifetime{ get_strong() };
        winrt::apartment_context context;
        co_await resume_background();

        std::vector<IAsyncOperation<App2AppCallResult>> calls;

        // Find all the packages that provide geolocation services and connect to them, asking each
        // to provide a response
        for (auto&& p : App2AppConnection::GetPackagesWithService(L"geolocation"))
        {
            if (auto conn = App2AppConnection::Connect(p.Id().FamilyName(), L"geolocation"))
            {
                calls.emplace_back(conn.InvokeAsync(PropertySet{}));
            }
        }

        // Wait for them all to complete, putting their output in the app view
        std::wstring results;
        for (auto&& i : calls)
        {
            auto res = co_await i;
            results.append(L"--");
            for (auto&& [k, v] : res.Result().as<IPropertySet>())
            {
                results.append(k).append(L"=").append(std::to_wstring(v.as<IPropertyValue>().GetDouble())).append(L";");
            }
        }

        co_await context;

        DebugOutputText().Text(results);
    }
}


winrt::fire_and_forget winrt::PluginCaller2::implementation::MainWindow::WindowQuery_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
{
    auto lifetime{ get_strong() };
    winrt::apartment_context context;
    co_await resume_background();

    auto matched = App2AppConnection::GetPackagesWithService(L"desktopinfo");
    if (matched.empty())
    {
        co_await context;
        DebugOutputText().Text(L"No packages with that service");
        co_return;
    }

    auto fam = matched.front().Id().FamilyName();
    auto conn = App2AppConnection::ConnectHttp(fam, L"desktopinfo");
    if (!conn)
    {
        co_await context;
        DebugOutputText().Text(std::wstring(L"Package ").append(fam).append(L" did not connect"));
        co_return;
    }

    HttpRequestMessage req{ HttpMethod::Get(), Uri{L"x-ms-app2app://something/window"} };
    auto resp = co_await conn.InvokeAsync(req);
    if (resp.IsSuccessStatusCode())
    {
        auto respBody = co_await resp.Content().ReadAsStringAsync();
        co_await context;
        DebugOutputText().Text(respBody);
    }
    else
    {
        co_await context;
        DebugOutputText().Text(std::to_wstring(static_cast<uint32_t>(resp.StatusCode())));
    }
}

winrt::fire_and_forget winrt::PluginCaller2::implementation::MainWindow::ForegroundInfo_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
{
    auto lifetime{ get_strong() };
    winrt::apartment_context context;
    co_await resume_background();

    auto matched = App2AppConnection::GetPackagesWithService(L"desktopinfo");
    if (matched.empty())
    {
        co_await context;
        DebugOutputText().Text(L"No packages with that service");
        co_return;
    }

    auto fam = matched.front().Id().FamilyName();
    auto conn = App2AppConnection::ConnectHttp(fam, L"desktopinfo");
    if (!conn)
    {
        co_await context;
        DebugOutputText().Text(std::wstring(L"Package ").append(fam).append(L" did not connect"));
        co_return;
    }

    HttpRequestMessage req{ HttpMethod::Get(), Uri{L"x-ms-app2app:something/window/foreground"} };
    auto resp = co_await conn.InvokeAsync(req);
    if (resp.IsSuccessStatusCode())
    {
        auto respBody = co_await resp.Content().ReadAsStringAsync();
        co_await context;
        DebugOutputText().Text(respBody);
    }
    else
    {
        co_await context;
        DebugOutputText().Text(std::to_wstring(static_cast<uint32_t>(resp.StatusCode())));
    }
}
