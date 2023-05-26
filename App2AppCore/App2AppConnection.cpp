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
            return connection_proxy::try_connect(foundClass.value());
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
            return connection_proxy::try_connect(clsid.value());
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
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetTypeInfo(
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ __RPC__deref_out_opt ITypeInfo** ppTInfo) noexcept override
        {
            *ppTInfo = nullptr;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetIDsOfNames(
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR* rgszNames,
            /* [range][in] */ __RPC__in_range(0, 16384) UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID* rgDispId) noexcept override
        {
            if ((riid != IID_NULL) || (cNames != 1) || (wcsicmp(rgszNames[0], L"invoke") != 0))
            {
                return E_NOTIMPL;
            }

            rgDispId[0] = 1;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_  DISPID dispIdMember,
            _In_  REFIID riid,
            _In_  LCID lcid,
            _In_  WORD wFlags,
            _In_  DISPPARAMS* pDispParams,
            _Out_opt_  VARIANT* pVarResult,
            _Out_opt_  EXCEPINFO* pExcepInfo,
            _Out_opt_  UINT* puArgErr) noexcept override
        {
            if ((dispIdMember != 1) || (riid != IID_NULL) || (wFlags != DISPATCH_METHOD) || 
                !pDispParams || (pDispParams->cArgs != 1) ||
                (pDispParams->rgvarg[0].vt != VT_UNKNOWN))
            {
                return E_NOTIMPL;
            }


        }
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
                return winrt::make_self<adapt_dispatch>(m_delegate())->QueryInterface(riid, ppvObject);
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
    };

    void App2AppConnection::RegisterHost(winrt::guid const& hostId, RequestConnectionHostDelegate delegate)
    {
        auto lock = std::lock_guard(m_lock);
    }

    void App2AppConnection::DeregisterHost(winrt::guid const& hostId)
    {
        auto lock = std::lock_guard(m_lock);
        auto f = m_cookies.find(hostId);
        if (f != m_cookies.end())
        {
            ::CoRevokeClassObject(f->second);
            m_cookies.erase(f);
        }
    }
}
