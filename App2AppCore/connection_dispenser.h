#pragma once

/*
* Provides a factory to hand out an IDispatch-based wrapper around the host app's
* IApp2AppConnection generator delegate.  The "Adapter" type must implement IDispatch.
*/
template<class Adapter, class Delegate> struct connection_dispenser : winrt::implements<connection_dispenser<Adapter, Delegate>, ::IClassFactory>
{
    connection_dispenser(Delegate d) : m_delegate(d) {}

    HRESULT STDMETHODCALLTYPE CreateInstance(::IUnknown* pOuter, REFIID riid, void** ppvObject) noexcept override
    {
        if (pOuter)
        {
            return CLASS_E_NOAGGREGATION;
        }

        try
        {
            return winrt::make_self<Adapter>(m_delegate())->QueryInterface(riid, ppvObject);
        }
        catch (winrt::hresult_error const& e)
        {
            return e.code();
        }
    }

    HRESULT LockServer(BOOL fLockServer)
    {
        if (fLockServer)
        {
            ++winrt::get_module_lock();
        }
        else
        {
            --winrt::get_module_lock();
        }

        return S_OK;
    }

    Delegate m_delegate;
};
