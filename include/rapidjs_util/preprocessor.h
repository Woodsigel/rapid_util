// Copyright (C) 2025 Liu Wu. All rights reserved.
//
// Licensed under the zlib License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/Zlib
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.


#pragma once

#include <string>
#include <list>
#include <vector>
#include <array>
#include <type_traits>
#include <optional>
#include <memory>

namespace rapidjson_util {

namespace detail {


template<typename... Types>
struct TypeList {
};

template<typename... Types>
constexpr auto make_typelist(Types&&...) {
    return TypeList<Types...> {};
}

template<typename F,
    template<typename ...> typename TypeList,
    typename ...Descriptors>
void for_each(const TypeList<Descriptors...>& , F&& f) {
    ((..., std::invoke(std::forward<F>(f), Descriptors{})));
}

template<typename Struct>
struct Descriptor {
    static constexpr bool is_describable = false;
    static constexpr TypeList<> member_descriptors{};
};


template<typename T>
using remove_const_and_reference_t = 
    std::remove_const_t<std::remove_reference_t<T>>;


template<typename T>
struct member_type;

template<typename T, typename C>
struct member_type<T C::*> {
    using type = T;
};

template<typename T>
using member_type_t = typename member_type<T>::type;


template<bool Const, typename T>
struct maybe_add_const {
    using type = std::conditional_t<Const, const T, T>;
};


template<template<typename> typename Wrapper, typename T>
struct is_wrapper : std::false_type {};

template<template<typename> typename Wrapper, typename T>
struct is_wrapper<Wrapper, Wrapper<T>> : std::true_type {};


template<template<typename> typename Wrapper, typename T>
struct remove_wrapper {
    using type = T;
};

template<template<typename> typename Wrapper, typename T>
struct remove_wrapper<Wrapper, Wrapper<T>> {
    using type = T;
};

template<template<typename> typename Wrapper, typename T>
struct remove_wrapper<Wrapper, const Wrapper<T>> {
    using type = const T;
};


template<typename T>
struct is_std_optional {
private:
    template<typename T>
    struct inner : is_wrapper<std::optional, T> {};

public:
    static constexpr bool value = 
        inner<remove_const_and_reference_t<T>>::value;
};

template<typename T>
constexpr bool is_std_optional_v = typename is_std_optional<T>::value;


template<typename T>
struct remove_std_optional {
private:
    template<typename T>
    struct inner : remove_wrapper<std::optional, T> {};

public:
    using type = typename inner<T>::type;
};

template<typename T>
using remove_std_optional_t = typename remove_std_optional<T>::type;


template<typename T>
struct is_std_shared_ptr {
private:
    template<typename T>
    struct inner : is_wrapper<std::shared_ptr, T> {};

public:
    static constexpr bool value =
        inner<remove_const_and_reference_t<T>>::value;
};

template<typename T>
constexpr bool is_std_shared_ptr_v = typename is_std_shared_ptr<T>::value;


template<typename T>
struct remove_std_shared_ptr {
private:
    template<typename T>
    struct inner : remove_wrapper<std::shared_ptr, T> {};

public:
    using type = typename inner<T>::type;
};

template<typename T>
using remove_std_shared_ptr_t = typename remove_std_shared_ptr<T>::type;


template<typename T>
struct remove_shared_optional {
private:
    template<typename T>
    struct inner {
        using type = T;
    };

    template<typename T>
    struct inner<std::shared_ptr<T>> 
        : remove_wrapper<std::shared_ptr, T> {};

    template<typename T>
    struct inner<const std::shared_ptr<T>> 
        : remove_wrapper<std::shared_ptr, const T> {};

    template<typename T>
    struct inner<std::optional<T>> 
        : remove_wrapper<std::optional, T> {};

    template<typename T>
    struct inner<const std::optional<T>> 
        : remove_wrapper<std::optional, const T> {};

public:
    using type = typename inner<T>::type;
};

template<typename T>
using remove_shared_optional_t = typename remove_shared_optional<T>::type;


template<typename T>
struct is_string_literal : std::false_type {};

template<size_t N>
struct is_string_literal<const char(&)[N]> : std::true_type {};

template<typename T>
constexpr bool is_string_literal_v = is_string_literal<T>::value;


template<typename T>
constexpr bool is_js_primitive_type_v = std::disjunction_v<
    std::is_same<T, int>, std::is_same<T, unsigned>,
    std::is_same<T, int64_t>, std::is_same<T, uint64_t>,
    std::is_same<T, float>, std::is_same<T, double>, 
    std::is_same<T, bool>, std::is_same<T, std::string>
>;

template<typename T>
constexpr bool is_jsonable_primitive_type_v =
    is_js_primitive_type_v<std::remove_cv_t<remove_shared_optional_t<T>>>;

template<typename T>
constexpr bool is_jsonable_struct_v =
    Descriptor<std::remove_cv_t<remove_shared_optional_t<T>>>::is_describable;


template<typename T>
struct is_jsonable_fixed_array {
private:
    template<typename T, typename = void>
    struct inner : std::false_type {};

    template<typename Elem, std::size_t N>
    struct inner<std::array<Elem, N>>
        : std::bool_constant<is_jsonable_primitive_type_v<Elem> || 
                             is_jsonable_struct_v<Elem>> 
    {};

    template<typename Elem, std::size_t N>
    struct inner<const std::array<Elem, N>> : inner<std::array<Elem, N>> {};

public:
    constexpr static bool value = 
        inner<remove_shared_optional_t<T>>::value;
};

template<typename Array>
constexpr bool is_jsonable_fixed_array_v = 
    is_jsonable_fixed_array<Array>::value;


template<typename T>
struct is_jsonable_vector {
private:
    template<typename T, typename = void>
    struct inner : std::false_type {};

    template<typename Elem, typename Alloc>
    struct inner<std::vector<Elem, Alloc>>
        : std::bool_constant<is_jsonable_primitive_type_v<Elem> || 
                             is_jsonable_struct_v<Elem>> 
    {};

    template<typename Elem, typename Alloc>
    struct inner<const std::vector<Elem, Alloc>> 
        : inner< std::vector<Elem, Alloc>> {};

public:
    constexpr static bool value = 
        inner<remove_shared_optional_t<T>>::value;
};


template<typename T>
struct is_jsonable_list {
private:
    template<typename T, typename = void>
    struct inner : std::false_type {};

    template<typename Elem, typename Alloc>
    struct inner<std::list<Elem, Alloc>>
        : std::bool_constant<is_jsonable_primitive_type_v<Elem> || 
                             is_jsonable_struct_v<Elem>> {};

    template<typename Elem, typename Alloc>
    struct inner<const std::list<Elem, Alloc>> 
        : inner<std::list<Elem, Alloc>> {};

public:
    constexpr static bool value = 
        inner<remove_shared_optional_t<T>>::value;
};
 

template<typename Container>
struct is_jsonable_dynamic_array
    : std::bool_constant<is_jsonable_list<Container>::value || 
                         is_jsonable_vector<Container>::value> 
{};

template<typename Container>
constexpr bool is_jsonable_dynamic_array_v = 
    is_jsonable_dynamic_array<Container>::value;


template<typename T>
constexpr bool is_jsonable_sequential_container_v = 
    is_jsonable_fixed_array_v<T> || is_jsonable_dynamic_array_v<T>;


template<typename Container>
struct can_hold_null_elem {
private:
    template<typename T, typename = void>
    struct inner : std::false_type {};

    template<
            template<typename, typename> typename Container, 
            typename Alloc, 
            typename U
            >
    struct inner<Container<std::optional<U>, Alloc>,
        std::enable_if_t<is_jsonable_dynamic_array_v<Container<std::optional<U>, Alloc>>>>
        : std::true_type {};

    template<
             template<typename, size_t> typename Array,
             size_t N, 
             typename U
             >
    struct inner<Array<std::optional<U>, N>,
        std::enable_if_t<is_jsonable_fixed_array_v<Array<std::optional<U>, N>>>>
        : std::true_type 
    {};

    template<
             template<typename, typename> typename Container, 
             typename Alloc, 
             typename U
            >
    struct inner<Container<std::shared_ptr<U>, Alloc>,
        std::enable_if_t<is_jsonable_dynamic_array_v<Container<std::shared_ptr<U>, Alloc>>>>
        : std::true_type {};

    template<
             template<typename, size_t> typename Array, 
             size_t N, 
             typename U
             >
    struct inner<Array<std::shared_ptr<U>, N>,
        std::enable_if_t<is_jsonable_fixed_array_v<Array<std::shared_ptr<U>, N>>>>
        : std::true_type {};

public:
    static constexpr bool value =
        inner<remove_const_and_reference_t<remove_shared_optional_t<Container>>>::value;
};


template<typename T>
struct is_std_tuple : std::false_type {};

template<typename... TupleArgs>
struct is_std_tuple<std::tuple<TupleArgs...>> : std::true_type {};

template<typename... TupleArgs>
struct is_std_tuple<const std::tuple<TupleArgs...>> : std::true_type {};

template<typename T>
constexpr bool is_std_tuple_v = is_std_tuple<T>::value;


template<typename T>
struct is_jsonable_tuple {
private:
    template<typename T>
    struct inner {
        static constexpr bool value = false;
    };

    template<typename First, typename... Rest>
    struct inner<std::tuple<First, Rest...>> {
    private:
        using F = std::remove_cv_t<remove_shared_optional_t<First>>;

        static constexpr bool check_first() {
            if constexpr (is_std_tuple_v<F>)
                return inner<F>::value;
            else
                return is_jsonable_primitive_type_v<F>       ||
                       is_jsonable_sequential_container_v<F> ||
                       is_jsonable_struct_v<F>;
        }

        static constexpr bool check_remaining() {
            if constexpr (sizeof...(Rest) == 0)
                return true;
            else
                return inner<std::tuple<Rest...>>::value;
        }

    public:
        static constexpr bool value = check_first() && check_remaining();
    };

public:
    static constexpr bool value = 
        inner<std::remove_cv_t<remove_shared_optional_t<T>>>::value;
};

template<typename T>
constexpr bool is_jsonable_tuple_v = is_jsonable_tuple<T>::value;


template<typename T>
constexpr bool is_json_serializable_v =
    (is_jsonable_primitive_type_v<T> || is_jsonable_sequential_container_v<T> ||
     is_jsonable_tuple_v<T>          || is_jsonable_struct_v<T>)
     &&
    (!std::is_pointer_v<T> && !std::is_reference_v<T> &&
     !std::is_const_v<T>);

}  // namespace detail
}  // namespace rapidjson_util 


#define RAPIDJSON_UTIL_STRINGIFY(x) RAPIDJSON_UTIL_STRINGIFY_I(x)
#define RAPIDJSON_UTIL_STRINGIFY_I(x) #x


#define RAPIDJSON_UTIL_UNPACK(...) __VA_ARGS__


#define RAPIDJSON_UTIL_EXPAND(x) x


#define RAPIDJSON_UTIL_CALL(F, C, x) F(C, x)

#define RAPIDJSON_UTIL_FOR_EACH_0(F, C) static_assert(false, "Members to be described cannot be empty.");
#define RAPIDJSON_UTIL_FOR_EACH_1(F, C, x) RAPIDJSON_UTIL_CALL(F, C, x)
#define RAPIDJSON_UTIL_FOR_EACH_2(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_1(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_3(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_2(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_4(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_3(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_5(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_4(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_6(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_5(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_7(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_6(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_8(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_7(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_9(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_8(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_10(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_9(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_11(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_10(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_12(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_11(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_13(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_12(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_14(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_13(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_15(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_14(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_16(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_15(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_17(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_16(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_18(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_17(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_19(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_18(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_20(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_19(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_21(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_20(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_22(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_21(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_23(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_22(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_24(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_23(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_25(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_24(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_26(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_25(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_27(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_26(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_28(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_27(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_29(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_28(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_30(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_29(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_31(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_30(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_32(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_31(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_33(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_32(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_34(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_33(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_35(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_34(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_36(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_35(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_37(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_36(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_38(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_37(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_39(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_38(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_40(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_39(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_41(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_40(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_42(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_41(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_43(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_42(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_44(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_43(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_45(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_44(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_46(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_45(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_47(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_46(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_48(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_47(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_49(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_48(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_50(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_49(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_51(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_50(F, C, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_52(F, C, x, ...) RAPIDJSON_UTIL_CALL(F, C, x),  RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_FOR_EACH_51(F, C, __VA_ARGS__))


#define RAPIDJSON_UTIL_EXTRACT(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,  _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, V, ...) V


#define RAPIDJSON_UTIL_FOR_EACH(F,  ...)                                                                                 \
      RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_EXTRACT(__VA_ARGS__,                                    \
                                                                 RAPIDJSON_UTIL_FOR_EACH_52,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_51,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_50,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_49,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_48,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_47,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_46,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_45,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_44,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_43,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_42,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_41,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_40,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_39,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_38,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_37,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_36,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_35,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_34,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_33,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_32,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_31,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_30,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_29,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_28,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_27,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_26,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_25,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_24,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_23,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_22,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_21,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_20,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_19,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_18,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_17,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_16,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_15,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_14,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_13,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_12,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_11,                             \
																 RAPIDJSON_UTIL_FOR_EACH_10,                             \
                                                                 RAPIDJSON_UTIL_FOR_EACH_9,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_8,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_7,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_6,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_5,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_4,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_3,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_2,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_1,                              \
                                                                 RAPIDJSON_UTIL_FOR_EACH_0))(F, __VA_ARGS__))   

#define RAPIDJSON_UTIL_STRIP_COMMAS_0()
#define RAPIDJSON_UTIL_STRIP_COMMAS_1(x) x
#define RAPIDJSON_UTIL_STRIP_COMMAS_2(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_1(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_3(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_2(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_4(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_3(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_5(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_4(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_6(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_5(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_7(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_6(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_8(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_7(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_9(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_8(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_10(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_9(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_11(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_10(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_12(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_11(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_13(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_12(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_14(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_13(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_15(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_14(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_16(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_15(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_17(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_16(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_18(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_17(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_19(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_18(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_20(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_19(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_21(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_20(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_22(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_21(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_23(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_22(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_24(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_23(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_25(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_24(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_26(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_25(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_27(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_26(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_28(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_27(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_29(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_28(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_30(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_29(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_31(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_30(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_32(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_31(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_33(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_32(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_34(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_33(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_35(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_34(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_36(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_35(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_37(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_36(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_38(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_37(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_39(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_38(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_40(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_39(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_41(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_40(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_42(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_41(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_43(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_42(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_44(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_43(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_45(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_44(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_46(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_45(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_47(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_46(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_48(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_47(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_49(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_48(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_50(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_49(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_51(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_50(__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_52(x, ...) RAPIDJSON_UTIL_EXPAND(x RAPIDJSON_UTIL_STRIP_COMMAS_51(__VA_ARGS__))

#define RAPIDJSON_UTIL_STRIP_COMMAS(...)                                                                           \
      RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_EXTRACT(__VA_ARGS__, ~,                           \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_52,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_51,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_50,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_49,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_48,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_47,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_46,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_45,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_44,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_43,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_42,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_41,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_40,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_39,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_38,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_37,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_36,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_35,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_34,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_33,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_32,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_31,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_30,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_29,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_28,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_27,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_26,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_25,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_24,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_23,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_22,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_21,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_20,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_19,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_18,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_17,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_16,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_15,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_14,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_13,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_12,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_11,                   \
																 RAPIDJSON_UTIL_STRIP_COMMAS_10,                   \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_9,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_8,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_7,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_6,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_5,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_4,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_3,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_2,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_1,                    \
                                                                 RAPIDJSON_UTIL_STRIP_COMMAS_0))(__VA_ARGS__))   



#define RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE(C, member) \
        static_assert(rapidjson_util::detail::is_json_serializable_v<rapidjson_util::detail::member_type_t<decltype(&C::member)>>, "Member variable types must be non-const and compatible with JSON.");

#define RAPIDJSON_UTIL_ASSERT_MEMBERS_ARE_SERIALIZABLE(C, members) \
        RAPIDJSON_UTIL_STRIP_COMMAS(RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE, C, RAPIDJSON_UTIL_UNPACK members))


#define RAPIDJSON_UTIL_MEMBER_META(C, member)                                                    \
	[]{ struct rapidjsonUtilDesc {                                                               \
             static constexpr auto pointer() noexcept { return &C::member; }                     \
             static constexpr auto name() noexcept { return RAPIDJSON_UTIL_STRINGIFY(member); }  \
    }; return rapidjsonUtilDesc{}; }  () 

#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMPL(C, members)  template<> struct rapidjson_util::detail::Descriptor<C> { \
     	static constexpr bool is_describable = true;                                                                \
        static constexpr auto member_descriptors = make_typelist(                                                   \
                       RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_MEMBER_META, C, RAPIDJSON_UTIL_UNPACK members));      \
        };


#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS(C, members)                                                                           \
        static_assert(std::is_class_v<C>, "RAPIDJSON_UTIL_DESCRIBE_MEMBERS should only be used with class or struct types");  \
        RAPIDJSON_UTIL_ASSERT_MEMBERS_ARE_SERIALIZABLE(C, members)                                                            \
        RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMPL(C, members)      



// Verify whether given aliases are string literals; C is unused (to satisfy RAPIDJSON_UTIL_FOR_EACH's interface).
#define RAPIDJSON_UTIL_ASSERT_ALIAS_IS_STRING_LITERAL(C, memAliasPair) RAPIDJSON_UTIL_ASSERT_ALIAS_IS_STRING_LITERAL_I(RAPIDJSON_UTIL_UNPACK memAliasPair)
#define RAPIDJSON_UTIL_ASSERT_ALIAS_IS_STRING_LITERAL_I(...)           RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_ASSERT_ALIAS_IS_STRING_LITERAL_II(__VA_ARGS__))
#define RAPIDJSON_UTIL_ASSERT_ALIAS_IS_STRING_LITERAL_II(member, alias) static_assert(rapidjson_util::detail::is_string_literal_v<decltype(alias)>, "Alias must be a string literal");

#define RAPIDJSON_UTIL_CHECK_ALIASES_ARE_STRING_LITERALS(C, memAliasPairs)  \
        RAPIDJSON_UTIL_STRIP_COMMAS(RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_ASSERT_ALIAS_IS_STRING_LITERAL, C, RAPIDJSON_UTIL_UNPACK memAliasPairs))


#define RAPIDJSON_UTIL_ASSERT_MEMBER_WITH_ALIAS_IS_SERIALIZABLE(C, memAliasPair) RAPIDJSON_UTIL_ASSERT_MEMBER_WITH_ALIAS_IS_SERIALIZABLE_I(C, RAPIDJSON_UTIL_UNPACK memAliasPair)
#define RAPIDJSON_UTIL_ASSERT_MEMBER_WITH_ALIAS_IS_SERIALIZABLE_I(C, ...)           RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_ASSERT_MEMBER_WITH_ALIAS_IS_SERIALIZABLE_II(C, __VA_ARGS__))
#define RAPIDJSON_UTIL_ASSERT_MEMBER_WITH_ALIAS_IS_SERIALIZABLE_II(C, member, alias) RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE(C, member)

#define RAPIDJSON_UTIL_ASSERT_MEMBERS_WITH_ALIAS_ARE_SERIALIZABLE(C, memAliasPairs)  \
        RAPIDJSON_UTIL_STRIP_COMMAS(RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_ASSERT_MEMBER_WITH_ALIAS_IS_SERIALIZABLE, C, RAPIDJSON_UTIL_UNPACK memAliasPairs))


#define RAPIDJSON_UTIL_MEMBER_ALIAS_META(C, memAliasPairs) RAPIDJSON_UTIL_MEMBER_ALIAS_META_I(C, RAPIDJSON_UTIL_UNPACK memAliasPairs)
#define RAPIDJSON_UTIL_MEMBER_ALIAS_META_I(C, ...)        RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_MEMBER_ALIAS_META_II(C, __VA_ARGS__))
#define RAPIDJSON_UTIL_MEMBER_ALIAS_META_II(C, member, alias)                 \
	[]{ struct rapidjsonUtilDesc {                                            \
             static constexpr auto pointer() noexcept { return &C::member; }  \
             static constexpr auto name() noexcept { return alias; }          \
    }; return rapidjsonUtilDesc{}; }  () 

#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS_IMPL(C, memAliasPairs)  template<> struct rapidjson_util::detail::Descriptor<C> {  \
        static constexpr bool is_describable = true;                                                                                  \
        static constexpr auto member_descriptors = make_typelist(                                                                     \
                       RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_MEMBER_ALIAS_META, C, RAPIDJSON_UTIL_UNPACK memAliasPairs));            \
        };

#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS(C, memAliasPairs)                                                                     \
        static_assert(std::is_class_v<C>, "RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS should only be used with class or struct types");  \
        RAPIDJSON_UTIL_CHECK_ALIASES_ARE_STRING_LITERALS(C, memAliasPairs)                                                               \
        RAPIDJSON_UTIL_ASSERT_MEMBERS_WITH_ALIAS_ARE_SERIALIZABLE(C, memAliasPairs)                                                      \
        RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS_IMPL(C, memAliasPairs)      