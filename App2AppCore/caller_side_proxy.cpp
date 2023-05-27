#include "pch.h"
#include "caller_side_proxy.h"
#include "App2AppCallResult.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;

    /*
    * This type is used on the /caller/ side of the app2app connection, providing a thing that looks
    * like an IApp2AppConnection instance sitting on top of the IDispatch real control layer. Pack
    * the incoming property set into a single VARIANTARG parameter, then call IDispatch::Invoke with
    * that.
    */
    IAsyncOperation<App2App::App2AppCallResult> caller_side_proxy::InvokeAsync(IPropertySet args)
    {
        // Someday we'll implement real cancellation here...
        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();

        // Run the dispatch call
        auto result = CallAndConvert<IPropertySet>(args);

        if (std::holds_alternative<winrt::hresult>(result))
        {
            co_return App2App::App2AppCallResult{App2App::App2AppCallResultStatus::Failed, std::get<winrt::hresult>(result), nullptr};
        }
        else
        {
            co_return App2App::App2AppCallResult{App2App::App2AppCallResultStatus::Completed, S_OK, std::get<IPropertySet>(result)};
        }
    }

    /*
    * CoCreate the id and ask it for IDispatch as the communication channel. If that succeeds, query
    * for the special "invoke" name, and then wrap a new IApp2AppConnection adapter around it.
    */
    App2App::IApp2AppConnection caller_side_proxy::try_connect(winrt::guid const& id)
    {
        if (auto conn = winrt::try_create_instance<::IDispatch>(id, CLSCTX_LOCAL_SERVER))
        {
            auto result = winrt::make_self<caller_side_proxy>();
            result->m_connection = std::move(conn);
            if (result->GetNames({ L"call",L"args" }))
            {
                return *result;
            }
        }

        return nullptr;
    }

}