#pragma once
#include <variant>

namespace winrt
{
    struct caller_proxy_base
    {
        std::variant<winrt::hresult, wil::unique_variant> MakeCall(winrt::Windows::Foundation::IUnknown const& object)
        {
            // Populate the args structure with the ABI of the incoming property set. Property sets are
            // themselves marshalable so anything in there will be carried across in the usual way.
            VARIANTARG vtTemp{};
            vtTemp.vt = VT_UNKNOWN;
            vtTemp.punkVal = reinterpret_cast<::IUnknown*>(winrt::get_abi(object));
            DISPPARAMS params{};
            params.cArgs = 1;
            params.rgvarg = &vtTemp;

            wil::unique_variant result;
            UINT argErrorIndex = 0;
            EXCEPINFO exInfo{};
            HRESULT hr;

            if (FAILED_LOG(hr = m_connection->Invoke(m_callIds[0], IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &result, &exInfo, &argErrorIndex)))
            {
                return winrt::hresult{hr};
            }
            else
            {
                return std::move(result);
            }
        }

        template<typename T> std::variant<winrt::hresult, T>  CallAndConvert(winrt::Windows::Foundation::IUnknown const& object)
        {
            auto result = MakeCall(object);

            if (std::holds_alternative<winrt::hresult>(result))
            {
                return std::get<winrt::hresult>(result);
            }
            else
            {
                wil::unique_variant& v = std::get<wil::unique_variant>(result);
                if (v.vt == VT_UNKNOWN)
                {
                    return to_winrt<T>(v.punkVal);
                }
                else if (v.vt == VT_DISPATCH)
                {
                    return to_winrt<T>(v.pdispVal);
                }
                else
                {
                    return winrt::hresult{E_UNEXPECTED};
                }
            }
        }

        bool GetNames(std::array<LPOLESTR, 2> invokeNames)
        {
            if (FAILED_LOG(m_connection->GetIDsOfNames(IID_NULL, invokeNames.data(), static_cast<UINT>(invokeNames.size()), LOCALE_USER_DEFAULT, m_callIds)))
            {
                return false;
            }

            LPOLESTR closeNames[] = { L"close" };
            LOG_IF_FAILED_MSG(m_connection->GetIDsOfNames(IID_NULL, closeNames, 1, LOCALE_USER_DEFAULT, &m_closeId), "No close method on this type");

            return true;
        }

        void Close()
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

        com_ptr<::IDispatch> m_connection;
        DISPID m_callIds[2]{ -1,-1 };
        DISPID m_closeId{ -1 };
    };
}