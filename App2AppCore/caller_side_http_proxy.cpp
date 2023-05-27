#include "pch.h"
#include "caller_side_http_proxy.h"
#include "App2AppCallResult.h"

namespace winrt
{
    using namespace App2App;
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
    using namespace Windows::Web::Http;

    /*
    * This type is used on the /caller/ side of the app2app connection, providing a thing that looks
    * like an IApp2AppHttpConnection instance sitting on top of the IDispatch real control layer. Pack
    * the incoming property set into a single VARIANTARG parameter, then call IDispatch::Invoke with
    * that.
    */
    IAsyncOperation<HttpResponseMessage> caller_side_http_proxy::InvokeAsync(HttpRequestMessage args)
    {
        // Someday we'll implement real cancellation here...
        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();

        auto result = CallAndConvert<HttpResponseMessage>(args);

        if (std::holds_alternative<HttpResponseMessage>(result))
        {
            co_return std::get<HttpResponseMessage>(result);
        }
        else
        {
            HttpResponseMessage resp;
            resp.StatusCode(HttpStatusCode::ServiceUnavailable);
            resp.Headers().Insert(L"hresult", std::to_wstring(std::get<winrt::hresult>(result).value));
            co_return resp;
        }
    }

    /*
    * CoCreate the id and ask it for IDispatch as the communication channel. If that succeeds, query
    * for the special "invokehttp" name, and then wrap a new IApp2AppHttpConnection adapter around it.
    */
    IApp2AppHttpConnection caller_side_http_proxy::try_connect(winrt::guid const& id)
    {
        if (auto conn = winrt::try_create_instance<::IDispatch>(id, CLSCTX_LOCAL_SERVER))
        {
            auto result = winrt::make_self<caller_side_http_proxy>();
            result->m_connection = std::move(conn);
            if (result->GetNames({ L"callhttp",L"args" }))
            {
                return *result;
            }
        }

        return nullptr;
    }

}