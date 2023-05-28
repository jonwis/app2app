#pragma once
#include <App2AppProviderInfo.g.h>

namespace winrt::App2App::implementation
{
    struct App2AppProviderInfo : App2AppProviderInfoT<App2AppProviderInfo>
    {
        App2AppProviderInfo(Windows::ApplicationModel::AppExtensions::AppExtension const& extension)
        {
            // ree
        }

        wil::single_threaded_property<Windows::ApplicationModel::Package> Package{nullptr};
        wil::single_threaded_property<winrt::hstring> Service;

        wil::single_threaded_property<winrt::guid> ActivationId;
        wil::single_threaded_property<winrt::hstring> AppExecAlias;
        wil::single_threaded_property<winrt::hstring> DescriptionFilePath;
        wil::single_threaded_property<winrt::hstring> InterfaceFilePath;
        wil::single_threaded_property<App2AppConnectionStyleKind> ConnectionStyle;

        Windows::Foundation::IInspectable Connect()
        {
            // ree
        }
    };
}