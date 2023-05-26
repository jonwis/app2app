#include "pch.h"
#include "wil/com.h"
#include "ConnectionProxy.h"
#include "App2AppCallResult.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Collections;


    inline IAsyncOperation<App2App::App2AppCallResult> caller_side_proxy::InvokeAsync(IPropertySet args)
    {
        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();

        // Initially, we stuff the property set into a VARIANT and throw that across the boundary
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

        if (SUCCEEDED(hr))
        {
            IPropertySet resultSet;

            if (result.vt == VT_UNKNOWN)
            {
                com_ptr<::IUnknown> resultUnk;
                winrt::copy_from_abi(resultUnk, result.punkVal);
                resultSet = resultUnk.as<IPropertySet>();
            }
            else
            {
                resultSet = {};
            }

            co_return winrt::make<App2App::implementation::App2AppCallResult>(App2App::App2AppCallResultStatus::Completed, winrt::hresult{}, resultSet);
        }
        else
        {
            co_return winrt::make<App2App::implementation::App2AppCallResult>(App2App::App2AppCallResultStatus::Failed, hr, nullptr);
        }
    }

    inline void caller_side_proxy::Close()
    {
        m_connection = nullptr;
    }

    App2App::IApp2AppConnection caller_side_proxy::try_connect(winrt::guid const& id)
    {
        if (auto conn = winrt::try_create_instance<::IDispatch>(id, CLSCTX_LOCAL_SERVER))
        {
            auto proxy = winrt::make_self<caller_side_proxy>();
            LPOLESTR names[] = { L"invoke" };
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