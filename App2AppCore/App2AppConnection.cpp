#include "pch.h"
#include "App2AppConnection.h"
#include "App2AppConnection.g.cpp"

namespace winrt
{
    using namespace Windows::ApplicationModel::AppExtensions;
    using namespace Windows::Management::Deployment;
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
}

namespace winrt::App2App::implementation
{
    std::optional<winrt::guid> FindClassId(hstring const& packageFamilyName, hstring const& service)
    {
        auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
        for (auto&& c : catalog.FindAllAsync().get())
        {
            if ((service == c.Id()) && (c.Package().Id().FamilyName() == packageFamilyName))
            {
                auto props = c.GetExtensionPropertiesAsync().get();

                if (auto activation = props.TryLookup(L"Activation").try_as<IPropertySet>())
                {
                    if (auto cid = activation.TryLookup(L"ClassId").try_as<IPropertyValue>())
                    {
                        return winrt::guid{ cid.GetString() };
                    }
                }
            }
        }

        return std::nullopt;
    }

    struct connection_proxy : winrt::implements<connection_proxy, IApp2AppConnection>
    {
        IApp2AppConnection m_connection;

        IAsyncOperation<App2AppCallResult> InvokeAsync(IPropertySet args)
        {
            auto cancel = co_await winrt::get_cancellation_token();
            cancel.enable_propagation();

            try
            {
                co_return co_await m_connection.InvokeAsync(args);
            }
            catch (winrt::hresult_error const& err)
            {
                co_return App2App::App2AppCallResult{ App2AppCallResultStatus::Failed, err.code(), nullptr};
            }
        }

        void Close()
        {
            m_connection.Close();
        }

        auto Closed(TypedEventHandler<IApp2AppConnection, IInspectable> const& e)
        {
            return m_closing.add(e);
        }

        auto Closed(winrt::event_token const& t)
        {
            return m_closing.remove(t);
        }

        winrt::event<TypedEventHandler<IApp2AppConnection, IInspectable>> m_closing;
    };

    winrt::App2App::IApp2AppConnection App2AppConnection::Connect(hstring const& packageFamilyName, hstring const& service)
    {
        auto clsid = FindClassId(packageFamilyName, service);

        if (!clsid)
        {
            // Probably should log something here
            return nullptr;
        }

        // Make the connection
        try
        {
            auto proxy = winrt::make_self<connection_proxy>();
            proxy->m_connection = winrt::try_create_instance<IApp2AppConnection>(clsid.value(), CLSCTX_LOCAL_SERVER);
            return *proxy;
        }
        catch (...)
        {
            return nullptr;
        }
    }
}
