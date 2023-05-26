#include "pch.h"
#include "MyRestWebPlugin.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Data::Json;
using namespace Windows::Web::Http;
using namespace App2App;
using namespace std::literals;

std::wstring_view uri_body_path(std::wstring_view uri)
{
    auto i = uri.find('/');
    if (i == uri.npos)
    {
        return {};
    }
    else
    {
        return uri.substr(i);
    }
}

JsonObject GetWindowDetails(HWND window)
{
    DWORD pid, tid;
    tid = GetWindowThreadProcessId(window, &pid);
    auto titleBuffer = std::make_unique<wchar_t[]>(2048);
    GetWindowTextW(window, titleBuffer.get(), 2048);

    JsonObject obj;
    obj.Insert(L"hwnd", JsonValue::CreateStringValue(std::to_wstring(reinterpret_cast<int64_t>(window))));
    obj.Insert(L"tid", JsonValue::CreateStringValue(std::to_wstring(tid)));
    obj.Insert(L"pid", JsonValue::CreateStringValue(std::to_wstring(pid)));
    obj.Insert(L"title", JsonValue::CreateStringValue(titleBuffer.get()));

    return obj;
}

template<typename T> void EnumWindows(T const& cb)
{
    ::EnumWindows([](HWND window, LPARAM context)
        {
            auto& m = *reinterpret_cast<decltype(&cb)>(context);
            return m(window);
        },
        (LPARAM)&cb);
}

IAsyncOperation<HttpResponseMessage> MyRestWebPlugin::InvokeAsync(HttpRequestMessage values)
{
    auto lifetime{ get_strong() };
    auto uri = values.RequestUri();
    auto method = values.Method();
    co_await resume_background();

    // We only support "get" operations, and only for the given protocol scheme.
    if ((method.ToString() != HttpMethod::Get().ToString()) || (uri.SchemeName() != L"x-ms-app2app"))
    {
        throw hresult_invalid_argument();
    }

    auto path = uri.Path();
    auto bodyPath = uri_body_path(path);
    HttpResponseMessage resp(HttpStatusCode::Ok);

    // Match some things
    if (bodyPath == L"/window/foreground")
    {
        auto deets = GetWindowDetails(GetForegroundWindow());
        resp.Content(HttpStringContent(deets.ToString()));
    }
    else if (bodyPath == L"/window")
    {
        JsonArray windows{};
        ::EnumWindows([&](HWND window)
            {
                windows.Append(GetWindowDetails(window));
                return TRUE;
            });
        resp.Content(HttpStringContent(windows.ToString()));
    }
    else
    {
        resp.StatusCode(HttpStatusCode::NotFound);
    }

    co_return resp;
}

void MyRestWebPlugin::Close()
{
}

void MyRestWebPlugin::Register()
{
    App2App::App2AppConnection::RegisterHttpHost(
        winrt::guid{ "77068367-f52e-4de9-a67d-bac1b68092b8" },
        [] { return winrt::make<MyRestWebPlugin>(); });
}
