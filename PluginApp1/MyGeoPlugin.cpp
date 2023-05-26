#include "pch.h"

#include <winrt/App2App.h>
#include <winrt/Windows.Devices.Geolocation.h>

#include "MyGeoPlugin.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace App2App;
using namespace Windows::Devices::Geolocation;

/*
* On invocation, use the Windows geolocation API to pull out some interesting values and stuff them
* into a response property set.
*/
IAsyncOperation<App2AppCallResult> MyGeoplugin::InvokeAsync(IPropertySet values)
{
    auto lifetime{ get_strong() };

    co_await resume_background();

    winrt::Windows::Devices::Geolocation::Geolocator loc{};
    auto where = co_await loc.GetGeopositionAsync();
    auto coord = where.Coordinate();

    ValueSet vs{};
    vs.Insert(L"altitude", box_value(coord.Altitude()));
    vs.Insert(L"accuracy", box_value(coord.Accuracy()));
    vs.Insert(L"latitude", box_value(coord.Latitude()));
    vs.Insert(L"longitude", box_value(coord.Longitude()));

    co_return App2AppCallResult(App2AppCallResultStatus::Completed, S_OK, vs);
}

/*
* Registers this type as an app2app plugin with a specific GUID type. Registration provides a
* delegate that - when the caller connects up to that object - constructs and returns a new
* instance of MyGeoPlugin, which implements IApp2AppConnection.
*/
void MyGeoplugin::Register()
{
    App2App::App2AppConnection::RegisterHost(
        winrt::guid{ "d69e1d12-c655-4378-80e1-48a9d649c35a" },
        [] { return winrt::make<MyGeoplugin>(); });
}

/*
* Raises an event when the client side issues a close call. Not currently super interesting.
*/
void MyGeoplugin::Close()
{
    m_closing(*this, nullptr);
}

winrt::event_token MyGeoplugin::Closed(TypedEventHandler<IApp2AppConnection, IInspectable> const& e)
{
    return m_closing.add(e);
}

void MyGeoplugin::Closed(winrt::event_token const& t)
{
    return m_closing.remove(t);
}

