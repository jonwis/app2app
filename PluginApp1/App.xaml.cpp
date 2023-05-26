// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include <winrt/App2App.h>

#include <MyGeoPlugin.h>
#include <MyRestWebPlugin.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace PluginApp1;
using namespace PluginApp1::implementation;

App::App()
{
    InitializeComponent();

    // During activation, tell the geoplugin to register.
    // Note: in the future, this could be just App2AppConnection.RegisterAll() which then uses
    // information from the current package manifest to perform all the registration required.
    MyGeoplugin::Register();
    MyRestWebPlugin::Register();

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

void App::OnLaunched(LaunchActivatedEventArgs const&)
{
    window = make<MainWindow>();
    window.Activate();
}

