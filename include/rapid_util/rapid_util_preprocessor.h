// Copyright (C) 2025 Liu Wu. All rights reserved.
//
// Licensed under the zlib License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/Zlib
//
// This software is provided ¡®as-is¡¯, without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.

#ifndef __RAPIDJSON_UTIL_PREPROCESSOR_H__
#define __RAPIDJSON_UTIL_PREPROCESSOR_H__

#include <string>
#include <list>
#include <vector>
#include <array>
#include <type_traits>
#include <optional>

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
    template<typename ...> typename List,
    typename ...Descriptors>
void for_each(const List<Descriptors...>& , F&& f) {
    ((..., std::invoke(std::forward<F>(f), Descriptors{})));
}

template<typename Struct>
struct Descriptor {
    static constexpr bool is_describable = false;
    static constexpr TypeList<> member_descriptors{};
};


template<typename T>
using remove_const_and_reference_t = std::remove_const_t<std::remove_reference_t<T>>;


template<typename T>
struct member_type;

template<typename T, typename C>
struct member_type<T C::*> {
    using type = T;
};

template<typename T>
using member_type_t = typename member_type<T>::type;


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


template<typename T>
struct is_std_optional_impl : is_wrapper<std::optional, T> {};

template<typename T>
struct is_std_optional : is_std_optional_impl<remove_const_and_reference_t<T>> {};

template<typename T>
constexpr bool is_std_optional_v = typename is_std_optional<T>::value;


template<typename T>
struct remove_std_optional_impl : remove_wrapper<std::optional, T> {};

template<typename T>
struct remove_std_optional : remove_std_optional_impl<remove_const_and_reference_t<T>> {};

template<typename T>
using remove_std_optional_t = typename remove_std_optional<T>::type;


// Use for debugging type inspection
template<typename Type>
struct type_displayer {
};


template<typename T>
constexpr bool is_json_primitive_core_type_v = std::disjunction_v<std::is_same<T, int>,
                                                             std::is_same<T, int8_t>,
                                                             std::is_same<T, int32_t>,
                                                             std::is_same<T, int64_t>,
                                                             std::is_same<T, uint64_t>,
                                                             std::is_same<T, bool>,
                                                             std::is_same<T, std::string>,
                                                             std::is_same<T, float>,
                                                             std::is_same<T, double>>;
template<typename T>
constexpr bool is_json_primitive_type_v = is_json_primitive_core_type_v<remove_std_optional_t<T>>;

template<typename T>
constexpr bool is_json_serializable_primitive_type_v =  is_json_primitive_type_v<T>
                                                        && !std::is_pointer_v<T> 
                                                        && !std::is_reference_v<T> 
                                                        && !std::is_const_v<T>;


template<typename T>
constexpr bool is_describable_struct_v = Descriptor<std::remove_reference_t<remove_std_optional_t<T>>>::is_describable;


template<typename T, typename = void>
struct is_json_serializable_fixed_array_impl : std::false_type {};

template<typename Elem, std::size_t N>
struct is_json_serializable_fixed_array_impl<std::array<Elem, N>>
    : std::bool_constant<is_json_serializable_primitive_type_v<Elem> || is_describable_struct_v<Elem>> {};

template<typename T>
struct is_json_serializable_fixed_array
    : is_json_serializable_fixed_array_impl<std::remove_reference_t<remove_std_optional_t<T>>> {};

template<typename Array>
constexpr bool is_json_serializable_fixed_array_v = is_json_serializable_fixed_array<Array>::value;


template<typename T, typename = void>
struct is_json_serializable_vector_impl : std::false_type {};

template<typename Elem, typename Alloc>
struct is_json_serializable_vector_impl<std::vector<Elem, Alloc>>
    : std::bool_constant<is_json_serializable_primitive_type_v<Elem> || is_describable_struct_v<Elem>> {};

template<typename T>
struct is_json_serializable_vector
    : is_json_serializable_vector_impl< std::remove_reference_t<remove_std_optional_t<T>>> {};


template<typename T, typename = void>
struct is_json_serializable_list_impl : std::false_type {};

template<typename Elem, typename Alloc>
struct is_json_serializable_list_impl<std::list<Elem, Alloc>>
      : std::bool_constant<is_json_serializable_primitive_type_v<Elem> || is_describable_struct_v<Elem>> {};

template<typename T>
struct is_json_serializable_list
    : is_json_serializable_list_impl<std::remove_reference_t<remove_std_optional_t<T>>> {};


template<typename Container>
struct is_json_serializable_dynamic_array
    : std::bool_constant<is_json_serializable_list<Container>::value || 
                         is_json_serializable_vector<Container>::value > {};

template<typename Container>
constexpr bool is_json_serializable_dynamic_array_v = is_json_serializable_dynamic_array<Container>::value;


template<typename T>
constexpr bool is_json_serializable_sequential_container_v = is_json_serializable_fixed_array_v<T> || is_json_serializable_dynamic_array_v<T>;


template<typename T, typename = void>
struct has_optional_elements_impl : std::false_type {};

template<template<typename, typename> typename Container, typename Alloc, typename U>
struct has_optional_elements_impl<Container<std::optional<U>, Alloc>,
                               std::enable_if_t<is_json_serializable_dynamic_array_v<Container<std::optional<U>, Alloc>>>>
    : std::true_type {};

template<template<typename, size_t> typename Array, size_t N, typename U>
struct has_optional_elements_impl<Array<std::optional<U>, N>,
                               std::enable_if_t<is_json_serializable_fixed_array_v<Array<std::optional<U>, N>>>>
    : std::true_type {};

template<typename Container>
struct has_optional_elements_impl<std::optional<Container>,
                               std::enable_if_t<has_optional_elements_impl<Container>::value>>
                               : std::true_type {};

template<typename Container>
struct has_std_optional_elements :
    has_optional_elements_impl<remove_const_and_reference_t<Container>> {};


template<typename T>
struct is_std_tuple : std::false_type {};

template<typename... TupleArgs>
struct is_std_tuple<std::tuple<TupleArgs...>> : std::true_type {};

template<typename T>
constexpr bool is_std_tuple_v = is_std_tuple<T>::value;


template<typename T>
struct is_json_serializable_tuple_impl {
    static constexpr bool value = false;
};

template<typename First, typename... Remaining >
struct is_json_serializable_tuple_impl<std::tuple<First, Remaining...>> {
private:
    static constexpr bool check_first() {
        if constexpr (is_std_tuple_v<First>) 
            return is_json_serializable_tuple_impl<First>::value;
        else 
            return is_json_serializable_primitive_type_v<First> ||
                   is_json_serializable_sequential_container_v<First> ||
                   is_describable_struct_v<First>;
    }

    static constexpr bool check_remaining() {
        if constexpr (sizeof...(Remaining) == 0) 
            return true;
        else 
            return is_json_serializable_tuple_impl<std::tuple<Remaining...>>::value;
    }

public:
    static constexpr bool value = check_first() && check_remaining();
};

template<typename T>
struct is_json_serializable_tuple {
    static constexpr bool value = is_json_serializable_tuple_impl<std::remove_reference_t<remove_std_optional_t<T>>>::value;
};

template<typename T>
constexpr bool is_json_serializable_tuple_v = is_json_serializable_tuple<T>::value;


template<typename T>
constexpr bool is_json_serializable_v = is_json_serializable_primitive_type_v<T> || is_json_serializable_sequential_container_v<T>
                                        || is_json_serializable_tuple_v<T> || is_describable_struct_v<T>;

}  // namespace detail
}  // namespace rapidjson_util 

#define RAPIDJSON_UTIL_EMPTY


#define RAPIDJSON_UTIL_STRINGIFY(x) RAPIDJSON_UTIL_STRINGIFY_I(x)
#define RAPIDJSON_UTIL_STRINGIFY_I(x) #x


#define RAPIDJSON_UTIL_UNPACK(...) __VA_ARGS__


#define RAPIDJSON_UTIL_EXPAND(x) x


#define RAPIDJSON_UTIL_CAT(a, b) RAPIDJSON_UTIL_CAT_I(a, b)
#define RAPIDJSON_UTIL_CAT_I(a, b) a ## b 


#define RAPIDJSON_UTIL_EVAL(...) RAPIDJSON_UTIL_EVAL128(__VA_ARGS__)
#define RAPIDJSON_UTIL_EVAL128(...) RAPIDJSON_UTIL_EVAL64(RAPIDJSON_UTIL_EVAL64(__VA_ARGS__))
#define RAPIDJSON_UTIL_EVAL64(...) RAPIDJSON_UTIL_EVAL32(RAPIDJSON_UTIL_EVAL32(__VA_ARGS__))
#define RAPIDJSON_UTIL_EVAL32(...) RAPIDJSON_UTIL_EVAL16(RAPIDJSON_UTIL_EVAL16(__VA_ARGS__))
#define RAPIDJSON_UTIL_EVAL16(...) RAPIDJSON_UTIL_EVAL8(RAPIDJSON_UTIL_EVAL8(__VA_ARGS__))
#define RAPIDJSON_UTIL_EVAL8(...) RAPIDJSON_UTIL_EVAL4(RAPIDJSON_UTIL_EVAL4(__VA_ARGS__))
#define RAPIDJSON_UTIL_EVAL4(...) RAPIDJSON_UTIL_EVAL2(RAPIDJSON_UTIL_EVAL2(__VA_ARGS__))
#define RAPIDJSON_UTIL_EVAL2(...) RAPIDJSON_UTIL_EVAL1(RAPIDJSON_UTIL_EVAL1(__VA_ARGS__))
#define RAPIDJSON_UTIL_EVAL1(...) __VA_ARGS__


#define RAPIDJSON_UTIL_GET_FIRST_ARG(a, ...) a
#define RAPIDJSON_UTIL_GET_SECOND_ARG(a, b, ...) b


#define RAPIDJSON_UTIL_IS_PROBE(...) RAPIDJSON_UTIL_EXPAND(RAPIDJSON_UTIL_GET_SECOND_ARG(__VA_ARGS__, 0))
#define RAPIDJSON_UTIL_PROBE ~, 1


#define RAPIDJSON_UTIL_NOT(x) RAPIDJSON_UTIL_IS_PROBE(RAPIDJSON_UTIL_CAT(RAPIDJSON_UTIL_NOT_ , x))
#define RAPIDJSON_UTIL_NOT_0 RAPIDJSON_UTIL_PROBE


#define RAPIDJSON_UTIL_BOOL(x) RAPIDJSON_UTIL_NOT(RAPIDJSON_UTIL_NOT(x))


#define RAPIDJSON_UTIL_IS_EMPTY(...) RAPIDJSON_UTIL_NOT(RAPIDJSON_UTIL_GET_FIRST_ARG( RAPIDJSON_UTIL_END_ARG_ __VA_ARGS__ )())
#define RAPIDJSON_UTIL_END_ARG_() 0


#define RAPIDJSON_UTIL_IF_ELSE(condition) RAPIDJSON_UTIL_CAT(RAPIDJSON_UTIL_IF_ELSE_, RAPIDJSON_UTIL_BOOL(condition) )
#define RAPIDJSON_UTIL_IF_ELSE_1(...)  __VA_ARGS__ RAPIDJSON_UTIL_IF_1_ELSE
#define RAPIDJSON_UTIL_IF_ELSE_0(...)              RAPIDJSON_UTIL_IF_0_ELSE
#define RAPIDJSON_UTIL_IF_1_ELSE(...) 
#define RAPIDJSON_UTIL_IF_0_ELSE(...) __VA_ARGS__


#define RAPIDJSON_UTIL_STRIP_COMMAS(x, ...) RAPIDJSON_UTIL_EVAL(RAPIDJSON_UTIL_STRIP_COMMAS_I(x, __VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_I(x, ...)  x                                                                 \
        RAPIDJSON_UTIL_IF_ELSE(RAPIDJSON_UTIL_IS_EMPTY(__VA_ARGS__))                                             \
        ()                                                                                                       \
        ( RAPIDJSON_UTIL_STRIP_COMMAS_II RAPIDJSON_UTIL_EMPTY () (__VA_ARGS__))
#define RAPIDJSON_UTIL_STRIP_COMMAS_II() RAPIDJSON_UTIL_STRIP_COMMAS_I


#define RAPIDJSON_UTIL_FOR_EACH(F, C, x, ...)   RAPIDJSON_UTIL_EVAL(RAPIDJSON_UTIL_FOR_EACH_I(F, C, x, __VA_ARGS__))
#define RAPIDJSON_UTIL_FOR_EACH_I(F, C, x, ...) F(C, x)                                                               \
        RAPIDJSON_UTIL_IF_ELSE(RAPIDJSON_UTIL_IS_EMPTY(__VA_ARGS__))                                                  \
        ( /* Do nothing, just terminate */ )                                                                          \
        (, RAPIDJSON_UTIL_FOR_EACH_II RAPIDJSON_UTIL_EMPTY () (F, C, __VA_ARGS__))             
#define RAPIDJSON_UTIL_FOR_EACH_II() RAPIDJSON_UTIL_FOR_EACH_I


#endif