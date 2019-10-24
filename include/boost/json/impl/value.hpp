//
// Copyright (c) 2018-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/json
//

#ifndef BOOST_JSON_IMPL_VALUE_HPP
#define BOOST_JSON_IMPL_VALUE_HPP

#include <boost/json/error.hpp>
#include <boost/json/detail/make_void.hpp>
#include <limits>
#include <type_traits>

namespace boost {
namespace json {

//----------------------------------------------------------

struct value::undo
{
    union
    {
        value old;
    };
    value* cur;

    explicit
    undo(value* cur_) noexcept
        : cur(cur_)
    {
        relocate(&old, *cur);
    }

    void
    commit() noexcept
    {
        old.~value();
        cur = nullptr;
    }

    ~undo()
    {
        if(cur)
            relocate(cur, old);
    }
};

//----------------------------------------------------------

namespace detail {

template<class T, class = void>
struct is_range : std::false_type
{
};

template<class T>
struct is_range<T, void_t<
    typename T::value_type,
    decltype(
        std::declval<T const&>().begin(),
        std::declval<T const&>().end()
    )>> : std::true_type
{
};

} // detail

//----------------------------------------------------------
//
// assign to value
//

// range
template<class T
    ,class = typename std::enable_if<
        detail::is_range<T>::value
        && ! std::is_same<typename T::value_type, char>::value
        && has_to_json<typename T::value_type>::value
            >::type
>
void
to_json(T const& t, value& v)
{
    v.reset(json::kind::array);
    for(auto const& e : t)
        v.as_array().push_back(e);
}

// string
inline
void
to_json(string_view t, value& v)
{
    v.emplace_string().assign(
        t.data(), t.size());
}

// string
inline
void
to_json(char const* t, value& v)
{
    v.emplace_string() = t;
}

// number
template<class T
    ,class = typename std::enable_if<
        std::is_constructible<number, T>::value &&
        ! std::is_same<number, T>::value &&
        ! std::is_convertible<T, storage_ptr>::value
            >::type
>
inline
void
to_json(T t, value& v)
{
    v.emplace_number() = t;
}

// bool
inline
void
to_json(bool b, value& v)
{
    v.emplace_bool() = b;
}

// null
inline
void
to_json(std::nullptr_t, value& v)
{
    v.emplace_null();
}

//----------------------------------------------------------
//
// assign value to
//

// integer

template<typename T
    ,class = typename std::enable_if<
        std::is_integral<T>::value>::type
>
void
from_json(T& t, value const& v)
{
    auto const& num = v.as_number();
    if(num.is_int64())
    {
        auto const rhs = num.get_int64();
        if( rhs > (std::numeric_limits<T>::max)() ||
            rhs < (std::numeric_limits<T>::min)())
            BOOST_JSON_THROW(system_error(
                error::integer_overflow));
        t = static_cast<T>(rhs);
    }
    else if(num.is_uint64())
    {
        auto const rhs = num.get_uint64();
        if(rhs > (std::numeric_limits<T>::max)())
            BOOST_JSON_THROW(system_error(
                error::integer_overflow));
        t = static_cast<T>(rhs);
    }
    else
    {
        BOOST_JSON_THROW(
            system_error(error::not_number));
    }
}

//----------------------------------------------------------

inline
void
swap(value& lhs, value& rhs)
{
    lhs.swap(rhs);
}


} // json
} // boost

#endif
