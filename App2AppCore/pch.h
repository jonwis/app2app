#pragma once
#include <unknwn.h>
#include <string_view>
#include <wil/cppwinrt.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Web.Http.h>
#include <wil/cppwinrt_helpers.h>
#include <wil/com.h>
#include <wil/resource.h>

using namespace std::literals;

template<typename To, typename From> To to_winrt(From* ptr)
{
    To result{ nullptr };
    winrt::check_hresult(ptr->QueryInterface(winrt::guid_of<To>(), winrt::put_abi(result)));
    return result;
}

// This is borrowed from WIL until it gets a release

namespace wil
{
    namespace details
    {
        template<typename T>
        struct single_threaded_property_storage
        {
            T m_value{};
            single_threaded_property_storage() = default;
            single_threaded_property_storage(const T& value) : m_value(value) {}
            operator T& () { return m_value; }
            operator T const& () const { return m_value; }
            template<typename Q> auto operator=(Q&& q)
            {
                m_value = wistd::forward<Q>(q);
                return *this;
            }
        };
    }

    template <typename T>
    struct single_threaded_property : std::conditional_t<std::is_scalar_v<T> || std::is_final_v<T>, wil::details::single_threaded_property_storage<T>, T>
    {
        single_threaded_property() = default;
        template <typename... TArgs> single_threaded_property(TArgs&&... value) : base_type(std::forward<TArgs>(value)...) {}

        using base_type = std::conditional_t<std::is_scalar_v<T> || std::is_final_v<T>, wil::details::single_threaded_property_storage<T>, T>;

        auto operator()()
        {
            return static_cast<T>(*this);
        }

        template<typename Q> auto& operator()(Q&& q)
        {
            *this = std::forward<Q>(q);
            return *this;
        }

        template<typename Q> auto& operator=(Q&& q)
        {
            static_cast<base_type&>(*this) = std::forward<Q>(q);
            return *this;
        }
    };

    template <typename T>
    struct single_threaded_rw_property : single_threaded_property<T>
    {
        using base_type = single_threaded_property<T>;
        template<typename... TArgs> single_threaded_rw_property(TArgs&&... value) : base_type(std::forward<TArgs>(value)...) {}

        using base_type::operator();

        // needed in lieu of deducing-this
        template<typename Q> auto& operator()(Q&& q)
        {
            return *this = std::forward<Q>(q);
        }

        // needed in lieu of deducing-this
        template<typename Q> auto& operator=(Q&& q)
        {
            base_type::operator=(std::forward<Q>(q));
            return *this;
        }
    };
}