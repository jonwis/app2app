#include "pch.h"
#include "App2AppConnection.h"
#include "App2AppConnection.g.cpp"
#include "ConnectionProxy.h"
#include "wil/resource.h"

namespace winrt
{
    using namespace Windows::ApplicationModel;
    using namespace Windows::ApplicationModel::AppExtensions;
    using namespace Windows::Management::Deployment;
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
}

namespace winrt::App2App::implementation
{
    std::optional<winrt::guid> FindClassIdInExtension(IPropertySet const& props)
    {
        if (auto activation = props.TryLookup(L"Activation").try_as<IPropertySet>())
        {
            if (auto cid = activation.TryLookup(L"ClassId").try_as<IPropertySet>())
            {
                if (auto val = cid.TryLookup(L"#text").try_as<IPropertyValue>())
                {
                    if (val.Type() == PropertyType::String)
                    {
                        return winrt::guid{val.GetString() };
                    }
                }
            }
        }

        return std::nullopt;
    }


    std::optional<winrt::guid> FindClassId(hstring const& packageFamilyName, hstring const& service)
    {
        auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
        for (auto&& c : catalog.FindAllAsync().get())
        {
            if ((service == c.Id()) && (c.Package().Id().FamilyName() == packageFamilyName))
            {
                if (auto found = FindClassIdInExtension(c.GetExtensionPropertiesAsync().get()))
                {
                    return found;
                }
            }
        }

        return std::nullopt;
    }

    com_array<Package> App2AppConnection::GetPackagesWithService(hstring const& service)
    {
        std::vector<Package> results;
        auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
        for (auto&& c : catalog.FindAllAsync().get())
        {
            if (service == c.Id())
            {
                results.emplace_back(c.Package());
            }
        }
        return com_array<Package>(std::move(results));
    }

    App2App::IApp2AppConnection App2AppConnection::ConnectToService(hstring const& service)
    {
        Package matching{ nullptr };

        auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
        std::optional<winrt::guid> foundClass;
        for (auto&& c : catalog.FindAllAsync().get())
        {
            if (service == c.Id())
            {
                if (foundClass = FindClassIdInExtension(c.GetExtensionPropertiesAsync().get()))
                {
                    break;
                }
            }
        }

        if (foundClass)
        {
            return connection_proxy::try_connect(foundClass.value());
        }
        else
        {
            return nullptr;
        }
    }

    winrt::App2App::IApp2AppConnection App2AppConnection::Connect(hstring const& packageFamilyName, hstring const& service)
    {
        if (auto clsid = FindClassId(packageFamilyName, service))
        {
            return connection_proxy::try_connect(clsid.value());
        }
        else
        {
            return nullptr;
        }
    }
}
