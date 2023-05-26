#include "pch.h"
#include "App2AppConnection.h"
#include "App2AppConnection.g.cpp"

#include "caller_side_proxy.h"
#include "app2appconn_adapter.h"
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
    /*
    * The AppExtension/Properties XML markup in a manifest is turned into a series of nested
    * property bags. Hunt around in there looking for /Activation/ClassId/#text, which
    * should be a string, and parse that. If no such value is found, return nothing.
    */
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

    /*
    * Callers might want to know which packages offer a specific named service. Enumerate the
    * apps that provide the service and return the array of matching Package objects to the
    * caller who might want to reason over them more.
    */
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

    /*
    * Connects to the first app2app provder with the given name. Note that this is dangerous,
    * as service _names_ are not unique.  If the connection could not be made, return nullptr.
    */
    App2App::IApp2AppConnection App2AppConnection::ConnectToService(hstring const& service)
    {
        Package matching{ nullptr };

        auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
        for (auto&& c : catalog.FindAllAsync().get())
        {
            if (service == c.Id())
            {
                if (auto cid = FindClassIdInExtension(c.GetExtensionPropertiesAsync().get()))
                {
                    if (auto connection = caller_side_proxy::try_connect(*cid))
                    {
                        return connection;
                    }
                }
            }
        }

        return nullptr;
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

    std::mutex s_registerLock;
    std::map<winrt::guid, wil::unique_com_class_object_cookie> s_registrations;

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
    void App2AppConnection::RegisterHost(winrt::guid const& hostId, RequestConnectionHostDelegate delegate)
    {
        auto dispenser = winrt::make_self<connection_dispenser<app2appconnection_adapter, RequestConnectionHostDelegate>>(std::move(delegate));

        auto lock = std::lock_guard(s_registerLock);

        if (s_registrations.find(hostId) == s_registrations.end())
        {
            wil::unique_com_class_object_cookie cookie;
            winrt::check_hresult(::CoRegisterClassObject(hostId, dispenser.get(), CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie));
            s_registrations.emplace(hostId, std::move(cookie));
        }
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
