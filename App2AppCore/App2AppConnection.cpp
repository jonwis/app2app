#include "pch.h"
#include "App2AppConnection.h"
#include "App2AppProviderInfo.h"
#include "App2AppConnection.g.cpp"

#include "caller_side_proxy.h"
#include "caller_side_http_proxy.h"
#include "app2appconn_adapter.h"
#include "httpconn_adapter.h"
#include "connection_dispenser.h"

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
    std::vector<AppExtension> GetApp2AppExtensions()
    {
        auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
        auto all = catalog.FindAllAsync().get();
        // Should be to_vector(all) - see https://github.com/microsoft/wil/issues/328
        return { begin(all), end(all) };
    }

    std::vector<AppExtension> FindExtensionsBy(std::optional<hstring> const& packageName, std::optional<hstring> const& service)
    {
        std::vector<AppExtension> results;

        for (auto&& e : GetApp2AppExtensions())
        {
            auto package = e.Package();
            if (packageName && (package.Id().FamilyName() != *packageName))
            {
                continue;
            }

            if (service && (e.Id() != *service))
            {
                continue;
            }

            results.emplace_back(std::move(e));
        }

        return results;
    }

    /*
    * The AppExtension/Properties XML markup in a manifest is turned into a series of nested
    * property bags. Hunt around in there looking for /Activation/ClassId/#text, which
    * should be a string, and parse that. If no such value is found, return nothing.
    */
    std::optional<winrt::guid> FindClassIdInExtensionProperties(IPropertySet const& props)
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

    /*
    * Find the app extensions for a package, and then find the app2app extension with a given name.
    * The package family name is a durable identifier derived from an installed package and thus
    * unique-ish.
    * 
    * Unfortunately there's no way to directly open the extension catalog for a specific package,
    * so enumerate _all_ the extensions in the catalog and find the one whose identity matches.
    * Then within there, look for the /AppExtension/Id property that matches the desired service.
    * 
    * If none match, return nothing.
    */
    std::optional<winrt::guid> FindClassIdForPackageService(hstring const& packageFamilyName, hstring const& service)
    {
        for (auto&& c : GetApp2AppExtensions())
        {
            if ((service == c.Id()) && (c.Package().Id().FamilyName() == packageFamilyName))
            {
                if (auto found = FindClassIdInExtensionProperties(c.GetExtensionPropertiesAsync().get()))
                {
                    return found;
                }
            }
        }

        return std::nullopt;
    }

    /*
    * Connects to a service provided by a specific application. Returns null if the connection
    * cannot be made.
    */
    winrt::App2App::IApp2AppConnection App2AppConnection::Connect(hstring const& packageFamilyName, hstring const& service)
    {
        auto clsid = FindClassIdForPackageService(packageFamilyName, service);
        return clsid ? caller_side_proxy::try_connect(*clsid) : nullptr;
    }

    /*
    * Connects to a service provided by a specific application. Returns null if the connection
    * cannot be made.
    */
    winrt::App2App::IApp2AppHttpConnection App2AppConnection::ConnectHttp(hstring const& packageFamilyName, hstring const& service)
    {
        auto clsid = FindClassIdForPackageService(packageFamilyName, service);
        return clsid ? caller_side_http_proxy::try_connect(*clsid) : nullptr;
    }

    std::mutex s_registerLock;
    std::map<winrt::guid, wil::unique_com_class_object_cookie> s_registrations;

    void RegisterDispenser(winrt::guid const& hostId, winrt::com_ptr<IClassFactory> const& factory)
    {
        auto lock = std::lock_guard(s_registerLock);

        if (s_registrations.find(hostId) == s_registrations.end())
        {
            wil::unique_com_class_object_cookie cookie;
            winrt::check_hresult(::CoRegisterClassObject(hostId, factory.get(), CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie));
            s_registrations.emplace(hostId, std::move(cookie));
        }
    }

    /*
    * Clear the set of registered dispenser objects
    */
    void App2AppConnection::UnregisterAllHosts()
    {
        auto lock = std::lock_guard(s_registerLock);
        s_registrations.clear();
    }

    /*
    * Registers an IApp2AppConnection for a specific host ID. The delegate will be invoked when the
    * class ID is CoCreated by a caller process.
    */
    void App2AppConnection::RegisterHost(winrt::guid const& hostId, App2AppConnectionHostFactory delegate)
    {
        RegisterDispenser(hostId, winrt::make_self<connection_dispenser<app2appconnection_adapter, App2AppConnectionHostFactory>>(std::move(delegate)));
    }

    /*
    * Registers an IApp2AppConnection for a specific host ID. The delegate will be invoked when the
    * class ID is CoCreated by a caller process.
    */
    void App2AppConnection::RegisterHttpHost(winrt::guid const& hostId, App2AppHttpConnectionHostFactory delegate)
    {
        RegisterDispenser(hostId, winrt::make_self<connection_dispenser<httpconn_adapter, App2AppHttpConnectionHostFactory>>(std::move(delegate)));
    }

    /*
    * Returns all the service providers matching this service name
    */
    winrt::com_array<App2App::App2AppProviderInfo> App2AppConnection::GetServiceProviders(hstring const& serviceName)
    {
        std::vector<App2App::App2AppProviderInfo> results;
        auto extensions = FindExtensionsBy(std::nullopt, serviceName);
        std::transform(extensions.begin(), extensions.end(), std::back_inserter(results), [](AppExtension const& e) { return winrt::make<App2AppProviderInfo>(e); });
        return winrt::com_array(std::move(results));
    }

    /*
    * Returns all the services provided by a given package. Note that this _could_ be faster/easier
    * by finding and parsing the package manifest. This instead enumerates all of them and picks out
    * the one that matters
    */
    winrt::com_array<App2App::App2AppProviderInfo> App2AppConnection::GetPackageServices(hstring const& packageFamily, std::optional<winrt::hstring> const& serviceName)
    {
        std::vector<App2App::App2AppProviderInfo> results;
        auto extensions = FindExtensionsBy(packageFamily, serviceName);
        std::transform(extensions.begin(), extensions.end(), std::back_inserter(results), [](AppExtension const& e) { return winrt::make<App2AppProviderInfo>(e); });
        return winrt::com_array(std::move(results));
    }

    /*
    * Unregisters a host ID, dropping the delegate as well.
    */
    void App2AppConnection::DeregisterHost(winrt::guid const& hostId)
    {
        // Find the host registered for this class ID. Revoke it from COM and remove it
        // from the map. Removing from the map destroys the delegate as well.
        auto lock = std::lock_guard(s_registerLock);
        s_registrations.erase(hostId);
    }
}
