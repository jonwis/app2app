#include "pch.h"
#include "App2AppCallResult.h"
#include "app2appconn_adapter.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

// This isn't a real IDispatch type yet, so just return that there's no type info
IFACEMETHODIMP app2appconnection_adapter::GetTypeInfoCount(UINT* pctinfo) noexcept
{
    *pctinfo = 0;
    return E_NOTIMPL;
}

// This isn't a real IDispatch type yet, so just return that there's no type info
IFACEMETHODIMP app2appconnection_adapter::GetTypeInfo(UINT, LCID, ITypeInfo** ppTInfo) noexcept
{
    *ppTInfo = nullptr;
    return E_NOTIMPL;
}

const DISPID DISPID_call = 1;
const DISPID DISPID_args = 2;
const DISPID DISPID_close = 3;

// There's only one method that matters, "call"
IFACEMETHODIMP app2appconnection_adapter::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID, DISPID* rgDispId) noexcept
{
    if (riid != IID_NULL)
    {
        return E_NOTIMPL;
    }
    else if (cNames < 1)
    {
        return E_INVALIDARG;
    }

    if ((cNames == 2) && (L"call"sv == rgszNames[0]) && (L"args"sv == rgszNames[1]))
    {
        rgDispId[0] = DISPID_call;
        rgDispId[1] = DISPID_args;
    }
    else if ((cNames == 1) && (L"close"sv == rgszNames[0]))
    {
        rgDispId[0] = DISPID_close;
    }
    else
    {
        return DISP_E_UNKNOWNNAME;
    }

    return S_OK;
}

/*
* Adapt an incoming call from IDispatch-variant form to the style desired by IApp2AppConnection. The
* protocol is that the method being called is "1" ("call" from above) and that it has a single
* parameter containing an IPropertySet.
*/
IFACEMETHODIMP app2appconnection_adapter::Invoke(DISPID dispIdMember, REFIID riid, LCID, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) noexcept
{
    // Be strict about what we allow here for now
    if ((riid != IID_NULL) || (wFlags != DISPATCH_METHOD))
    {
        return E_INVALIDARG;
    }

    wil::assign_to_opt_param(pVarResult, {});
    wil::assign_to_opt_param(pExcepInfo, {});
    wil::assign_to_opt_param(puArgErr, {});

    try
    {
        if (dispIdMember == DISPID_call)
        {
            if (!pDispParams || (pDispParams->cArgs != 1) || (pDispParams->rgvarg[0].vt != VT_UNKNOWN) ||
                !pVarResult ||
                !m_conn)
            {
                return E_UNEXPECTED;
            }

            // Convert the incoming arguments back to an IPropertySet, then pass that along to InvokeAsync.
            // Note that here InvokeAsync could really be async, but IDispatch::Invoke is synchronous, so
            // the call waits for completion.
            auto response = m_conn.InvokeAsync(to_winrt<IPropertySet>(pDispParams->rgvarg[0].punkVal)).get();

            if (response.Status() == winrt::App2App::App2AppCallResultStatus::Completed)
            {
                // When the call completes, send the unknown response back to the caller if IDispatch::Invoke
                // as the one output parameter type. IApp2AppConnection returns a property set, so just stick
                // a reference to it into a VT_UNKNOWN variant type.
                pVarResult->vt = VT_UNKNOWN;
                winrt::copy_to_abi(response.Result(), reinterpret_cast<void*&>(pVarResult->punkVal));
                return S_OK;
            }
            else
            {
                return response.ExtendedError();
            }
        }
        else if (dispIdMember == DISPID_close)
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
            return E_UNEXPECTED;
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
