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

        // Populate the args structure with the ABI of the incoming property set. Property sets are
        // themselves marshalable so anything in there will be carried across in the usual way.
        VARIANTARG vtTemp{};
        vtTemp.vt = VT_UNKNOWN;
        vtTemp.punkVal = reinterpret_cast<::IUnknown*>(winrt::get_abi(args));
        DISPPARAMS params{};
        params.cArgs = 1;
        params.rgvarg = &vtTemp;

        wil::unique_variant result;
        UINT argErrorIndex = 0;
        EXCEPINFO exInfo{};

        HRESULT hr = m_connection->Invoke(m_mainId, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &result, &exInfo, &argErrorIndex);

        if (SUCCEEDED_LOG(hr))
        {
            // After the invoke, convert the one output argument (if present) back to the
            // IPropertySet tht the caller of _this_ interface expects to see.
            if (result.vt == VT_UNKNOWN)
            {
                co_return to_winrt<HttpResponseMessage>(result.punkVal);
            }
            else
            {
                co_return nullptr;
            }
        }
        else
        {
            HttpResponseMessage resp;
            resp.StatusCode(HttpStatusCode::ServiceUnavailable);
            resp.Headers().Insert(L"hresult", std::to_wstring(hr));
            co_return resp;
        }
    }

    void caller_side_http_proxy::Close()
    {
        m_connection = nullptr;
    }

    /*
    * CoCreate the id and ask it for IDispatch as the communication channel. If that succeeds, query
    * for the special "invokehttp" name, and then wrap a new IApp2AppHttpConnection adapter around it.
    */
    IApp2AppHttpConnection caller_side_http_proxy::try_connect(winrt::guid const& id)
    {
        if (auto conn = winrt::try_create_instance<::IDispatch>(id, CLSCTX_LOCAL_SERVER))
        {
            auto proxy = winrt::make_self<caller_side_http_proxy>();
            LPOLESTR names[] = { L"invokehttp" };
            conn->GetIDsOfNames(IID_NULL, names, _countof(names), LOCALE_USER_DEFAULT, &proxy->m_mainId);
            proxy->m_connection = conn;
            return *proxy;
        }
        else
        {
            return nullptr;
        }
    }

}