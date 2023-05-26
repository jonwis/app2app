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
        for (auto&& i : calls)
        {
            auto res = co_await i;
            for (auto&& [k, v] : res.Result().as<IPropertySet>())
            {
                printf("Key %ls, value %p", k.c_str(), std::addressof(v));
            }
        }

        co_await context;

        myButton().Content(box_value(L"Clicked"));
    }
}
