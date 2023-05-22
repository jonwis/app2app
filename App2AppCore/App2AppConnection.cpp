#include "pch.h"
#include "App2AppConnection.h"
#include "App2AppConnection.g.cpp"

namespace winrt::App2App::implementation
{
    winrt::App2App::IApp2AppConnection App2AppConnection::Connect(hstring const& packageFamilyName, hstring const& service)
    {
        throw hresult_not_implemented();
    }
}
