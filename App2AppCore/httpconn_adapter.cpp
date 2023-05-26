#include "pch.h"
#include "App2AppCallResult.h"
#include "httpconn_adapter.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Web::Http;

// This isn't a real IDispatch type yet, so just return that there's no type info
IFACEMETHODIMP httpconn_adapter::GetTypeInfoCount(UINT* pctinfo) noexcept
{
    *pctinfo = 0;
    return E_NOTIMPL;
}

// This isn't a real IDispatch type yet, so just return that there's no type info
IFACEMETHODIMP httpconn_adapter::GetTypeInfo(UINT, LCID, ITypeInfo** ppTInfo) noexcept
{
    *ppTInfo = nullptr;
    return E_NOTIMPL;
}

const DISPID did_invokehttp = 1;
const DISPID did_close = 2;

// There's only one method that matters, "invoke"
IFACEMETHODIMP httpconn_adapter::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID, DISPID* rgDispId) noexcept
{
    if (riid != IID_NULL)
    {
        return E_NOTIMPL;
    }

    for (uint32_t i = 0; i < cNames; ++i)
    {
        if (L"invokehttp"sv == rgszNames[i])
        {
            rgDispId[i] = did_invokehttp;
        }
        else if (L"close"sv == rgszNames[i])
        {
            rgDispId[i] = did_close;
        }
        else
        {
            rgDispId[i] = {};
        }
    }

    return S_OK;
}

/*
* Adapt an incoming call from IDispatch-variant form to the style desired by IApp2AppConnection. The
* protocol is that the method being called is "1" ("invoke" from above) and that it has a single
* parameter containing an IPropertySet.
*/
IFACEMETHODIMP httpconn_adapter::Invoke(DISPID dispIdMember, REFIID riid, LCID, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) noexcept
{
    // Be strict about what we allow here for now
    if ((riid != IID_NULL) || (wFlags != DISPATCH_METHOD) ||
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
        if (dispIdMember == did_invokehttp)
        {
            if (!m_conn)
            {
                return E_UNEXPECTED;
            }

            // Convert the incoming arguments back to an IPropertySet, then pass that along to InvokeAsync.
            // Note that here InvokeAsync could really be async, but IDispatch::Invoke is synchronous, so
            // the call waits for completion.
            auto response = m_conn.InvokeAsync(to_winrt<HttpRequestMessage>(pDispParams->rgvarg[0].punkVal)).get();

            // When the call completes, send the unknown response back to the caller if IDispatch::Invoke
            // as the one output parameter type. IApp2AppHttpConnection returns an HttpResponseMessage,
            // so send out a reference to that via a VT_UNKNOWN variant type.
            pVarResult->vt = VT_UNKNOWN;
            winrt::copy_to_abi(response, reinterpret_cast<void*&>(pVarResult->punkVal));
            return S_OK;
        }
        else if (dispIdMember == did_close)
        {
            if (m_conn)
            {
                m_conn.Close();
                m_conn = nullptr;
            }
            return S_OK;
        }
        else
        {
            return E_INVALIDARG;
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
