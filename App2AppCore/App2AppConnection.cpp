#include "pch.h"
#include "App2AppConnection.h"
#include "App2AppConnection.g.cpp"
#include "ConnectionProxy.h"
#include "wil/resource.h"

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


    std::optional<winrt::guid> FindClassId(hstring const& packageFamilyName, hstring const& service)
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

    App2App::IApp2AppConnection App2AppConnection::ConnectToService(hstring const& service)
    {
        Package matching{ nullptr };

        auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
        std::optional<winrt::guid> foundClass;
        for (auto&& c : catalog.FindAllAsync().get())
        {
            if (service == c.Id())
            {
                if (foundClass = FindClassIdInExtension(c.GetExtensionPropertiesAsync().get()))
                {
                    break;
                }
            }
        }

        if (foundClass)
        {
            return caller_side_proxy::try_connect(foundClass.value());
        }
        else
        {
            return nullptr;
        }
    }

    winrt::App2App::IApp2AppConnection App2AppConnection::Connect(hstring const& packageFamilyName, hstring const& service)
    {
        if (auto clsid = FindClassId(packageFamilyName, service))
        {
            return caller_side_proxy::try_connect(clsid.value());
        }
        else
        {
            return nullptr;
        }
    }

    struct adapt_dispatch : winrt::implements<adapt_dispatch, IDispatch>
    {
        HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
            /* [out] */ __RPC__out UINT* pctinfo) noexcept override
        {
            *pctinfo = 0;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetTypeInfo(
            /* [in] */ UINT,
            /* [in] */ LCID,
            /* [out] */ __RPC__deref_out_opt ITypeInfo** ppTInfo) noexcept override
        {
            *ppTInfo = nullptr;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetIDsOfNames(
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR* rgszNames,
            /* [range][in] */ __RPC__in_range(0, 16384) UINT cNames,
            /* [in] */ LCID,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID* rgDispId) noexcept override
        {
            if ((riid != IID_NULL) || (cNames != 1) || (L"invoke"sv != rgszNames[0]))
            {
                return E_NOTIMPL;
            }

            rgDispId[0] = 1;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_  DISPID dispIdMember,
            _In_  REFIID riid,
            _In_  LCID,
            _In_  WORD wFlags,
            _In_  DISPPARAMS* pDispParams,
            _Out_opt_  VARIANT* pVarResult,
            _Out_opt_  EXCEPINFO* pExcepInfo,
            _Out_opt_  UINT* puArgErr) noexcept override
        {
            if ((dispIdMember != 1) || (riid != IID_NULL) || (wFlags != DISPATCH_METHOD) ||
                !pDispParams || (pDispParams->cArgs != 1) || (pDispParams->rgvarg[0].vt != VT_UNKNOWN) ||
                !pVarResult)
            {
                return E_INVALIDARG;
            }

            wil::assign_to_opt_param(pVarResult, {});
            wil::assign_to_opt_param(pExcepInfo, {});
            wil::assign_to_opt_param(puArgErr, {});

            try
            {
                winrt::com_ptr<IUnknown> unk;
                winrt::copy_from_abi(unk, pDispParams->rgvarg[0].punkVal);
                auto response = m_conn.InvokeAsync(unk.as<IPropertySet>()).get();

                if (response.Status() == App2AppCallResultStatus::Completed)
                {
                    pVarResult->vt = VT_UNKNOWN;
                    winrt::copy_to_abi(response.Result(), reinterpret_cast<void*&>(pVarResult->punkVal));
                    return S_OK;
                }
                else
                {
                    return response.ExtendedError();
                }
            }
            catch (winrt::hresult_error const& hr)
            {
                return hr.code();
            }
            catch (...)
            {
                return E_FAIL;
            }
        }

        IApp2AppConnection m_conn;
    };

    struct connection_dispenser : winrt::implements<connection_dispenser, ::IClassFactory>
    {
        HRESULT STDMETHODCALLTYPE CreateInstance(::IUnknown* pOuter, REFIID riid, void** ppvObject) noexcept override
        {
            if (pOuter)
            {
                return CLASS_E_NOAGGREGATION;
            }

            try
            {
                auto adapter = winrt::make_self<adapt_dispatch>();
                adapter->m_conn = m_delegate();
                return adapter->QueryInterface(riid, ppvObject);
            }
            catch (winrt::hresult_error const& e)
            {
                return e.code();
            }
        }

        HRESULT LockServer(BOOL)
        {
            return S_OK;
        }

        RequestConnectionHostDelegate m_delegate;
        DWORD m_cookie{ 0 };
    };

    std::mutex s_registerLock;
    std::map<winrt::guid, winrt::com_ptr<connection_dispenser>> s_registrations;

    void App2AppConnection::UnregisterAllHosts()
    {
        auto lock = std::lock_guard(s_registerLock);
        for (auto&& [key, val] : s_registrations)
        {
            ::CoRevokeClassObject(val->m_cookie);
        }
        s_registrations.clear();
    }

    void App2AppConnection::RegisterHost(winrt::guid const& hostId, RequestConnectionHostDelegate delegate)
    {
        auto dispenser = winrt::make_self<connection_dispenser>();
        dispenser->m_delegate = delegate;

        auto lock = std::lock_guard(s_registerLock);

        if (s_registrations.find(hostId) == s_registrations.end())
        {
            winrt::check_hresult(::CoRegisterClassObject(hostId, dispenser.get(), CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &dispenser->m_cookie));
            s_registrations.emplace(hostId, std::move(dispenser));
        }
    }

    void App2AppConnection::DeregisterHost(winrt::guid const& hostId)
    {
        // Find the host registered for this class ID. Revoke it from COM and remove it
        // from the map. Removing from the map destroys the delegate as well.
        auto lock = std::lock_guard(s_registerLock);
        if (auto i = s_registrations.find(hostId); i != s_registrations.end())
        {
            ::CoRevokeClassObject(i->second->m_cookie);
            s_registrations.erase(i);
        }
    }
}
