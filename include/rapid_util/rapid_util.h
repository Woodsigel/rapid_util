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

#ifndef __SIMPLE_RAPID_JSON_UTIL_H__
#define __SIMPLE_RAPID_JSON_UTIL_H__

#include <type_traits>
#include "rapid_util_preprocessor.h"
#include "rapid_util_parser.h"


namespace rapidjson_util {

/**
 * @brief Serialize a C++ struct to JSON string
 *
 * @param s The struct instance to serialize, whose members are described by the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro
 * @return JSON string representation of the struct
 *
 * 
 * @code
 * struct Person {
 *     std::string name;
 *     int age;
 * };
 *
 * RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Person, (name, age))
 *
 * Person p{"John", 30};
 * std::string json = marshal(p);  // {"name":"John","age":30}
 * @endcode
 */
template<typename Struct> 
std::string marshal(const Struct& s) noexcept {
    return detail::marshalImp(s);
}

/**
 * @brief Deserialize a JSON string to populate a C++ struct
 *
 * @param json JSON string to parse and deserialize
 * @param s The struct instance to populate with deserialized data, whose members are described by the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro
 *
 *
 * @code
 * struct Person {
 *     std::string name;
 *     int age;
 * };
 * RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Person, (name, age))
 *
 * Person p;
 * unmarshal(R"({"name":"Alice","age":25})", p);
 * // p.name == "Alice", p.age == 25
 * @endcode
 */
template<typename Struct>
void unmarshal(std::string_view json, Struct& s) {
    return detail::unmarshalImp(json, s);
}

namespace detail {

enum WrapperType {
    None,
    StdSharedPtr
};

enum JsonSourceType {
    Struct,      // C++ struct/class
    Primitive,   // Basic C++ types
    Sequential,  // Containers (vector, list, array)
    Tuple        // std::tuple
};

template<typename T>
constexpr WrapperType wrapper_type_trait_v =
                               is_std_shared_ptr_v<T> ? WrapperType::StdSharedPtr : WrapperType::None;

template<size_t JsonSourceType, size_t WrapperType, bool isConstQualified>
struct JsonValueCreator;

template<size_t WrapperType, bool isConstQualified>
struct JsonValueCreator<JsonSourceType::Primitive, WrapperType, isConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonPrimitiveValue> create(T& value) {
        static_assert(is_json_serializable_primitive_type_v<std::remove_const_t<T>>);

        return std::make_shared<JsonPrimitiveValue>(&value);
    }
};

template<bool isConstQualified>
struct JsonValueCreator<JsonSourceType::Struct, WrapperType::None, isConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonObject> create(T& value) {
        static_assert(!is_std_shared_ptr_v<T>);

        return std::make_shared<JsonObject>(buildJsonTreeFrom(value));
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Struct, WrapperType::StdSharedPtr, true> {
    template<typename T>
    static std::shared_ptr<JsonSharedPtrObject> create(T& value) {
        static_assert(is_std_shared_ptr_v<T> && std::is_const_v<T>);

        if (value == nullptr)
            return std::make_shared<JsonSharedPtrObject>();
        else
            return  std::make_shared<JsonSharedPtrObject>(buildJsonTreeFrom(std::as_const(*value)));
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Struct, WrapperType::StdSharedPtr, false> {
    template<typename T>
    static std::shared_ptr<JsonSharedPtrObject> create(T& value) {
        static_assert(is_std_shared_ptr_v<T> && !std::is_const_v<T>);

        auto object = (value == nullptr) ? std::make_shared<JsonSharedPtrObject>() :
            std::make_shared<JsonSharedPtrObject>(buildJsonTreeFrom(*value));

        auto sharedPtrResetter = [&value]() { value.reset(); };
        auto sharedPtrReinitializer = [&value]()
        {
            using BaseType = remove_std_shared_ptr_t<T>;
            value = std::make_shared<BaseType>();

            auto object = JsonValueCreator<JsonSourceType::Struct, WrapperType::StdSharedPtr, false>::create(value);
            return object->getMembers();
        };

        object->setReferencedSharedPtrHandlers(sharedPtrReinitializer, sharedPtrResetter);

        return object;
    }
};

template<typename Sequence>
std::vector<std::shared_ptr<JsonValue>> convertToJsonValuesFromSeq(Sequence& sequence) {
    std::vector<std::shared_ptr<JsonValue>> elements;

    for (auto&& it : sequence) {
        using elemType = remove_const_and_reference_t<decltype(it)>;

        if constexpr (is_json_serializable_primitive_type_v<elemType>)
            elements.push_back(createJsonPrimitiveValueFrom(it));

        else if constexpr (is_describable_struct_v<elemType>)
            elements.push_back(createJsonObjectFrom(it));
    }

    return elements;
}

template<bool isConstQualified>
struct JsonValueCreator<JsonSourceType::Sequential, WrapperType::None, isConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonArray> create(T& sequence) {
        static_assert(!is_std_shared_ptr_v<T>);

        auto elements = convertToJsonValuesFromSeq(sequence);
        auto jsonArray = std::make_shared<JsonArray>(elements, has_shared_ptr_elements<T>::value);

        if constexpr (!std::is_const_v<T> && is_json_serializable_dynamic_array_v<T>)
            jsonArray->setArrayResizer([sequencePtr = &sequence](std::size_t newSize) {
            sequencePtr->resize(newSize);
            return  convertToJsonValuesFromSeq(*sequencePtr);
                });

        return jsonArray;
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Sequential, WrapperType::StdSharedPtr, true> {
    template<typename T>
    static std::shared_ptr<JsonSharedPtrArray> create(T& sequence) {
        static_assert(std::is_const_v<T> && is_std_shared_ptr_v<T>);

        bool hasSharedPtrElems = has_shared_ptr_elements<T>::value;
        if (nullptr == sequence)
            return std::make_shared<JsonSharedPtrArray>(hasSharedPtrElems);

        else {
            auto elements = convertToJsonValuesFromSeq(std::as_const(*sequence));
            return std::make_shared<JsonSharedPtrArray>(elements, hasSharedPtrElems);
        }
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Sequential, WrapperType::StdSharedPtr, false> {
    template<typename T>
    static std::shared_ptr<JsonSharedPtrArray> create(T& sequence) {
        static_assert(!std::is_const_v<T> && is_std_shared_ptr_v<T>);

        bool hasSharedPtrElems = has_shared_ptr_elements<T>::value;
        auto jsonArray = (sequence == nullptr) ? std::make_shared<JsonSharedPtrArray>(hasSharedPtrElems) :
            std::make_shared<JsonSharedPtrArray>(convertToJsonValuesFromSeq(*sequence), hasSharedPtrElems);


        auto sharedPtrReinitializer = [&sequence]()
        {
            using BaseType = remove_std_shared_ptr_t<T>;
            sequence = std::make_shared<BaseType>();

            return std::vector<std::shared_ptr<JsonValue>>{};
        };

        auto resizer = [&sequence, sharedPtrReinitializer](std::size_t newSize) {
            if (sequence == nullptr)
                sharedPtrReinitializer();

            sequence->resize(newSize);
            return  convertToJsonValuesFromSeq(*sequence); };

        auto sharedPtrResetter = [&sequence]() { sequence.reset(); };

        jsonArray->setArrayResizer(resizer);
        jsonArray->setReferencedSharedPtrHandlers(sharedPtrReinitializer, sharedPtrResetter);

        return jsonArray;
    }
};

template<typename Tuple>
std::vector <std::shared_ptr<JsonValue>> convertToJsonValuesFromTup(Tuple& tuple) {
    std::vector<std::shared_ptr<JsonValue>> elements;

    std::apply([&elements](auto&&... tupleArgs) {
        auto process = [&elements](auto&& arg) {
            using elemType = remove_const_and_reference_t<decltype(arg)>;

            if constexpr (is_json_serializable_primitive_type_v<elemType>)
                elements.push_back(createJsonPrimitiveValueFrom(arg));

            else if constexpr (is_describable_struct_v<elemType>)
                elements.push_back(createJsonObjectFrom(arg));

            else if constexpr (is_json_serializable_tuple_v<elemType>)
                elements.push_back(createJsonArrayFromTup(arg));

            else if constexpr (is_json_serializable_sequential_container_v<elemType>)
                elements.push_back(createJsonArrayFromSeq(arg));
        };

        (process(std::forward<decltype(tupleArgs)>(tupleArgs)), ...);
        }, tuple);

    return elements;
}

template<bool isConstQualified>
struct JsonValueCreator<JsonSourceType::Tuple, WrapperType::None, isConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonArray> create(T& tuple) {
        static_assert(!is_std_shared_ptr_v<T>);

        auto elements = convertToJsonValuesFromTup(tuple);
        return  std::make_shared<JsonArray>(elements);
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Tuple, WrapperType::StdSharedPtr, true> {
    template<typename T>
    static std::shared_ptr<JsonSharedPtrArray> create(T& tuple) {
        static_assert(std::is_const_v<T> && is_std_shared_ptr_v<T>);

        if (nullptr == tuple)
            return std::make_shared<JsonSharedPtrArray>();

        else {
            auto elements = convertToJsonValuesFromTup(std::as_const(*tuple));
            return std::make_shared<JsonSharedPtrArray>(elements);
        }
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Tuple, WrapperType::StdSharedPtr, false> {
    template<typename T>
    static std::shared_ptr<JsonSharedPtrArray> create(T& tuple) {
        static_assert(!std::is_const_v<T> && is_std_shared_ptr_v<T>);

        auto jsonArray = (tuple == nullptr) ? std::make_shared<JsonSharedPtrArray>() :
            std::make_shared<JsonSharedPtrArray>(convertToJsonValuesFromTup(*tuple));


        auto sharedPtrReinitializer = [&tuple]()
        {
            using BaseType = remove_std_shared_ptr_t<T>;
            tuple = std::make_shared<BaseType>();

            return convertToJsonValuesFromTup(*tuple);
        };

        auto sharedPtrResetter = [&tuple]() { tuple.reset(); };

        jsonArray->setReferencedSharedPtrHandlers(sharedPtrReinitializer, sharedPtrResetter);

        return jsonArray;
    }
};

template<typename T>
std::shared_ptr<JsonValue> createJsonPrimitiveValueFrom(T& value) {
    static_assert(is_json_serializable_primitive_type_v<std::remove_const_t<T>>);

    return JsonValueCreator<JsonSourceType::Primitive, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(value);
}

template<typename T>
std::shared_ptr<JsonValue> createJsonObjectFrom(T& value) {
    static_assert(is_describable_struct_v<std::remove_const_t<T>>);

    return JsonValueCreator<JsonSourceType::Struct, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(value);
}

template<typename T>
std::shared_ptr<JsonValue> createJsonArrayFromSeq(T& sequence) {
    static_assert(is_json_serializable_sequential_container_v<std::remove_const_t<T>>);

    return JsonValueCreator<JsonSourceType::Sequential, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(sequence);
}

template<typename T>
std::shared_ptr<JsonValue> createJsonArrayFromTup(T& tuple) {
    static_assert(is_json_serializable_tuple_v<std::remove_const_t<T>>);

    return JsonValueCreator<JsonSourceType::Tuple, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(tuple);
}

template<typename Desc>
std::string getMemberName(Desc descriptor) {
    return std::string(descriptor.name());
}

template<typename Struct, typename Desc>
decltype(auto) getMemberValueRef(Struct& s, Desc descriptor) {
    return s.*(descriptor.pointer());
}


template<typename Struct>
std::vector<JsonAttribute> buildJsonTreeFrom(Struct& s) {
    static_assert(is_describable_struct_v<std::remove_const_t<Struct>>, "Use the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro to declare serializable struct members");

    std::vector<JsonAttribute> members;

    auto descriptors = Descriptor<std::remove_const_t<Struct>>::member_descriptors;
    for_each(descriptors, [&s, &members](auto desc) {
        std::string name = getMemberName(desc);
        using MemberType = remove_const_and_reference_t<decltype(getMemberValueRef(s, desc))>;

        if constexpr (is_json_serializable_primitive_type_v<MemberType>)
            members.push_back(JsonAttribute{ name, createJsonPrimitiveValueFrom(getMemberValueRef(s, desc)) });

        else if constexpr (is_json_serializable_sequential_container_v<MemberType>)
            members.push_back(JsonAttribute{ name, createJsonArrayFromSeq(getMemberValueRef(s, desc)) });

        else if constexpr (is_json_serializable_tuple_v<MemberType>)
            members.push_back(JsonAttribute{ name, createJsonArrayFromTup(getMemberValueRef(s, desc)) });

        else if constexpr (is_describable_struct_v<MemberType>)
            members.push_back(JsonAttribute{ name, createJsonObjectFrom(getMemberValueRef(s, desc)) });
        });

    return members;
}

template<typename Struct>
std::string marshalImp(const Struct& s) {
    auto root = std::make_shared<JsonObject>(buildJsonTreeFrom(s));

    JsonWriter writer;
    return writer.witeToJson(root.get());
}

template<typename Struct>
void unmarshalImp(std::string_view json, Struct& s)  {
    JsonReader reader(json);

    auto root = std::make_shared<JsonObject>(buildJsonTreeFrom(s));
    reader.readFromJson(root.get());
}


#define RAPIDJSON_UTIL_CHECK_MEMBERS_ARE_SERIALIZABLE(C, members) \
        RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE, C, RAPIDJSON_UTIL_UNPACK members)

#define RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE(C, member) \
        static_assert(rapidjson_util::detail::is_json_serializable_v<rapidjson_util::detail::member_type_t<decltype(&C::member)>>, "Member variable types must be compatible with JSON value types.");


#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMP(C, members)  template<> struct rapidjson_util::detail::Descriptor<C> {     \
     	static constexpr bool is_describable = true;                                                                   \
        static constexpr auto member_descriptors = make_typelist(                                                      \
                      RAPIDJSON_UTIL_REMOVE_FIRST_ARG(RAPIDJSON_UTIL_FOR_EACH(                                         \
                          RAPIDJSON_UTIL_MEMBER_META, C, RAPIDJSON_UTIL_UNPACK members)));                             \
        };


#define RAPIDJSON_UTIL_MEMBER_META(C, member)                                                      \
	, []{ struct rapidjsonUtilDesc {                                                               \
             static constexpr auto pointer() noexcept { return &C::member; }                       \
             static constexpr auto name() noexcept { return RAPIDJSON_UTIL_STRINGIFY(member); }    \
    }; return rapidjsonUtilDesc{}; }  () 

}  // namespace detail
}  // namespace rapidjson_util 

#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS(C, members)                        \
        static_assert(std::is_class_v<C>);                                 \
        RAPIDJSON_UTIL_CHECK_MEMBERS_ARE_SERIALIZABLE(C, members)          \
        RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMP(C, members)


#endif