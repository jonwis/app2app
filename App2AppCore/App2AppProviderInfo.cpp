#include <pch.h>
#include "App2AppProviderInfo.h"
#include "App2AppProviderInfo.g.cpp"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
    using namespace Windows::ApplicationModel;
    using namespace Windows::ApplicationModel::AppExtensions;
}

namespace winrt::App2App::implementation
{
    winrt::hstring ReadStringProperty(IPropertySet const& props, winrt::array_view<std::wstring_view> path)
    {
        IPropertySet leaf = props;
        for (auto&& i : path)
        {
            if (!(leaf = leaf.TryLookup(i).try_as<IPropertySet>()))
            {
                break;
            }
        }

        hstring value;
        if (auto val = leaf.TryLookup(L"#text").as<IPropertyValue>())
        {
            value = val.GetString();
        }
        return value;
    }

    App2AppProviderInfo::App2AppProviderInfo(AppExtension const& extension)
    {
        Package = extension.Package();
        Service = extension.Id();

        auto props = extension.GetExtensionPropertiesAsync().get();
        if (auto activationInfo = props.TryLookup(L"Activation"))
        {
        }

        InterfaceFilePath = ReadStringProperty(props, L"Interface");
        DescriptionFilePath = ReadStringProperty(props, L"Description");
    }

    IInspectable winrt::App2App::implementation::App2AppProviderInfo::Connect()
    {
        // ree
    }
}

