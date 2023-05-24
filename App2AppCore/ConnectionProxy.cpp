#include "pch.h"
#include "ConnectionProxy.h"

namespace winrt
{
    inline Windows::Foundation::IAsyncOperation<App2App::App2AppCallResult> connection_proxy::InvokeAsync(Windows::Foundation::Collections::IPropertySet args)
    {
        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();

        try
        {
            co_return co_await m_connection.InvokeAsync(args);
        }
        catch (hresult_error const& err)
        {
            co_return App2App::App2AppCallResult{ App2App::App2AppCallResultStatus::Failed, err.code(), nullptr};
        }
    }

    inline void connection_proxy::Close()
    {
        m_connection.Close();
    }

    App2App::IApp2AppConnection connection_proxy::try_connect(winrt::guid const& id)
    {
        if (auto conn = winrt::try_create_instance<App2App::IApp2AppConnection>(id))
        {
            auto proxy = winrt::make_self<connection_proxy>();
            proxy->m_connection = conn;
            return *proxy;
        }
        else
        {
            return nullptr;
        }
    }

}