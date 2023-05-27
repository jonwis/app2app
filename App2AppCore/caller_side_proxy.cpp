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

        HRESULT hr = m_connection->Invoke(m_callIds[0], IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &result, &exInfo, &argErrorIndex);

        if (SUCCEEDED_LOG(hr))
        {
            // After the invoke, convert the one output argument (if present) back to the
            // IPropertySet tht the caller of _this_ interface expects to see.
            IPropertySet resultSet;

            if (result.vt == VT_UNKNOWN)
            {
                resultSet = to_winrt<IPropertySet>(result.punkVal);
            }

            co_return winrt::App2App::App2AppCallResult{ App2App::App2AppCallResultStatus::Completed, winrt::hresult{}, resultSet };
        }
        else
        {
            co_return winrt::App2App::App2AppCallResult{App2App::App2AppCallResultStatus::Failed, hr, nullptr};
        }
    }

    void caller_side_proxy::Close()
    {
        if (m_connection)
        {
            if (m_closeId == -1)
            {
                DISPPARAMS params{};
                wil::unique_variant result;
                UINT argErrorIndex;
                EXCEPINFO exInfo{};
                m_connection->Invoke(m_closeId, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &result, &exInfo, &argErrorIndex);
            }

            m_connection = nullptr;
        }
    }

    inline winrt::event_token caller_side_proxy::Closed(Windows::Foundation::TypedEventHandler<App2App::IApp2AppConnection, Windows::Foundation::IInspectable> const& e)
    {
        return m_closing.add(e);
    }

    inline void caller_side_proxy::Closed(event_token const& t)
    {
        return m_closing.remove(t);
    }

    /*
    * CoCreate the id and ask it for IDispatch as the communication channel. If that succeeds, query
    * for the special "invoke" name, and then wrap a new IApp2AppConnection adapter around it.
    */
    App2App::IApp2AppConnection caller_side_proxy::try_connect(winrt::guid const& id)
    {
        com_ptr<caller_side_proxy> result;

        if (auto conn = winrt::try_create_instance<::IDispatch>(id, CLSCTX_LOCAL_SERVER))
        {
            result = winrt::make_self<caller_side_proxy>();
            
            // Find the name of the "call" method, which has one parameter - "args"
            LPOLESTR callMethodNames[] = { L"call", L"args" };
            static_assert(_countof(callMethodNames) == _countof(result->m_callIds));
            if (FAILED_LOG(conn->GetIDsOfNames(IID_NULL, callMethodNames, _countof(callMethodNames), LOCALE_USER_DEFAULT, result->m_callIds)))
            {
                return nullptr;
            }

            // Try finding the name of the "close" method; which if it doesn't exist is fine
            LPOLESTR closeNames[] = { L"close" };
            LOG_IF_FAILED_MSG(conn->GetIDsOfNames(IID_NULL, closeNames, 1, LOCALE_USER_DEFAULT, &result->m_closeId), "No close method on this type");

            result->m_connection = std::move(conn);
        }

        return *result;
    }

}