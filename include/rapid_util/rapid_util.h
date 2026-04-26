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
 * @param s The struct instance to serialize, whose members are  
 *          described by the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro
 * 
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
    return detail::marshalImpl(s);
}


/**
 * @brief Deserialize a JSON string to populate a C++ struct
 *
 * @param json JSON string to parse and deserialize
 * @param s The struct instance to populate with deserialized data, whose 
 *          members are described by the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro
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
    return detail::unmarshalImpl(json, s);
}

namespace detail {


template<typename T>
std::shared_ptr<JsonValue> 
convertToJsonValue(T& memberRef);

template<typename Sequence>
std::vector<std::shared_ptr<JsonValue>> 
seqToJsonArrayElems(Sequence& sequence) {

    static_assert(is_json_serializable_sequential_container_v<Sequence>);

    std::vector<std::shared_ptr<JsonValue>> elements;

    for (auto&& item : sequence)
        elements.push_back(convertToJsonValue(item));
        
    return elements;
}

template<typename Tuple>
std::vector <std::shared_ptr<JsonValue>> 
tupleToJsonArrayElems(Tuple& tuple) {

    static_assert(is_json_serializable_tuple_v<Tuple>);

    std::vector<std::shared_ptr<JsonValue>> elements;

    std::apply([&elements](auto&&... tupleArgs) 
        {
            (..., (elements.push_back(convertToJsonValue(tupleArgs))));
        }, 
        tuple);

    return elements;
}


enum WrapperType {
    None,
    StdOptional
};

enum JsonSourceType {
    Struct,      // C++ struct/class
    Primitive,   // Basic C++ types
    Sequential,  // Containers (vector, list, array)
    Tuple        // std::tuple
};

template<typename T>
constexpr WrapperType 
wrapper_type_v = is_std_optional_v<T> ? WrapperType::StdOptional : 
                                        WrapperType::None;

template
<
    size_t JsonSourceType, 
    size_t WrapperType, 
    bool isConstQualified
>
struct JsonValueCreator;

template
<
    size_t UnusedWrapperType,
    bool UnusedIsConstQualified
>
struct JsonValueCreator
    <
     JsonSourceType::Primitive,
     UnusedWrapperType, 
     UnusedIsConstQualified
    > {
    
    template<typename T>
    static std::shared_ptr<JsonPrimitiveValue> 
    create(T& value) {
        static_assert(is_json_serializable_primitive_type_v<T>);

        return std::make_shared<JsonPrimitiveValue>(&value);
    }
};


template<bool unusedIsConstQualified>
struct JsonValueCreator
    <
    JsonSourceType::Struct, 
    WrapperType::None, 
    unusedIsConstQualified
    > {

    template<typename T>
    static std::shared_ptr<JsonObject> 
    create(T& value) {
        static_assert(!is_std_optional_v<T>);

        return std::make_shared<JsonObject>(buildJsonTreeFrom(value));
    }
};

template<>
struct JsonValueCreator
    <
    JsonSourceType::Struct, 
    WrapperType::StdOptional, 
    true
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableObject> 
    create(T& stdOptionalStruct) {

        static_assert(is_std_optional_v<T> && std::is_const_v<T>);

        if (stdOptionalStruct.has_value()) {
            auto attributes = buildJsonTreeFrom(std::as_const(stdOptionalStruct.value()));
            return std::make_shared<JsonNullableObject>(attributes);
        }

        else 
            return std::make_shared<JsonNullableObject>();
        
    }
};

template<>
struct JsonValueCreator
    <
    JsonSourceType::Struct, 
    WrapperType::StdOptional, 
    false
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableObject> 
    create(T& stdOptionalStruct) {
        static_assert(is_std_optional_v<T> && !std::is_const_v<T>);

        auto object = stdOptionalStruct.has_value()
            ? std::make_shared<JsonNullableObject>(buildJsonTreeFrom(stdOptionalStruct.value()))
            : std::make_shared<JsonNullableObject>();
                                           
        auto resetter = [&stdOptionalStruct]() { stdOptionalStruct.reset(); };

        auto reinitializer = [&stdOptionalStruct]() {
            using BaseType = remove_std_optional_t<T>;

            stdOptionalStruct = BaseType{};

            auto object = JsonValueCreator<
                JsonSourceType::Struct,
                WrapperType::StdOptional,
                false
            >::create(stdOptionalStruct);

            return object->getMembers();
        };

        object->setReferencedValueHandlers(reinitializer, resetter);

        return object;
    }
};


template<bool isConstQualified>
struct JsonValueCreator
    <
    JsonSourceType::Sequential, 
    WrapperType::None, 
    isConstQualified
    > {

    template<typename T>
    static std::shared_ptr<JsonArray> 
    create(T& sequence) {

        static_assert(!is_std_optional_v<T>);

        auto elements = seqToJsonArrayElems(sequence);
        auto jsonArray = std::make_shared<JsonArray>(elements, contain_std_optional_elements<T>::value);

        if constexpr (!isConstQualified && is_json_serializable_dynamic_array_v<T>) {
            jsonArray->setArrayResizer([&sequence](std::size_t newSize) {
                sequence.resize(newSize);
                return seqToJsonArrayElems(sequence);
                });
        }

        return jsonArray;
    }
};

template<>
struct JsonValueCreator
    <
    JsonSourceType::Sequential, 
    WrapperType::StdOptional, 
    true
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableArray> 
    create(T& sequence) {
        static_assert(is_std_optional_v<T> && std::is_const_v<T>);

        bool containOpt = contain_std_optional_elements<T>::value;

        if (sequence.has_value()) {
            auto elements = seqToJsonArrayElems(std::as_const(sequence.value()));
            return std::make_shared<JsonNullableArray>(elements, containOpt);
        }

        else 
            return std::make_shared<JsonNullableArray>(containOpt);
        
    }
};

template<>
struct JsonValueCreator
    <
    JsonSourceType::Sequential, 
    WrapperType::StdOptional, 
    false
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableArray> create(T& sequence) {
        static_assert(is_std_optional_v<T> &&  !std::is_const_v<T>);

        bool containOpt = contain_std_optional_elements<T>::value;
        auto jsonArray = sequence.has_value()
            ? std::make_shared<JsonNullableArray>(seqToJsonArrayElems(sequence.value()), containOpt)
            : std::make_shared<JsonNullableArray>(containOpt);

        auto reinitializer = [&sequence]() {
            using BaseType = remove_std_optional_t<T>;
            sequence = BaseType{};
            return std::vector<std::shared_ptr<JsonValue>>{};
        };

        auto resizer = [&sequence, reinitializer](std::size_t newSize) {
            if (!sequence.has_value()) {
                reinitializer();
            }
            sequence->resize(newSize);
            return seqToJsonArrayElems(sequence.value());
        };

        auto resetter = [&sequence]() { sequence.reset(); };

        jsonArray->setArrayResizer(resizer);
        jsonArray->setReferencedValueHandlers(reinitializer, resetter);
        
        return jsonArray;
    }
};


template<bool UnusedIsConstQualified>
struct JsonValueCreator
    <
    JsonSourceType::Tuple, 
    WrapperType::None, 
    UnusedIsConstQualified
    > {

    template<typename T>
    static std::shared_ptr<JsonArray> 
    create(T& tuple) {
        static_assert(!is_std_optional_v<T>);

        auto elements = tupleToJsonArrayElems(tuple);

        return  std::make_shared<JsonArray>(elements);
    }
};

template<>
struct JsonValueCreator
    <
    JsonSourceType::Tuple, 
    WrapperType::StdOptional, 
    true
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableArray> 
    create(T& stdOptionalTup) {
        static_assert(is_std_optional_v<T> && std::is_const_v<T>);

        if (stdOptionalTup.has_value()) {
            auto elements = tupleToJsonArrayElems(std::as_const(stdOptionalTup.value()));
            return std::make_shared<JsonNullableArray>(elements);
        }
            
        else
            return std::make_shared<JsonNullableArray>();
    }
};

template<>
struct JsonValueCreator
    <
    JsonSourceType::Tuple, 
    WrapperType::StdOptional, 
    false
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableArray> 
    create(T& stdOptionalTup) {

        static_assert(is_std_optional_v<T> && !std::is_const_v<T>);

        auto jsonArray = stdOptionalTup.has_value()
            ? std::make_shared<JsonNullableArray>(tupleToJsonArrayElems(stdOptionalTup.value()))
            : std::make_shared<JsonNullableArray>();

        auto reinitializer = [&stdOptionalTup]() {
            using BaseType = remove_std_optional_t<T>;
            stdOptionalTup = BaseType{};
            return tupleToJsonArrayElems(stdOptionalTup.value());
        };

        auto resetter = [&stdOptionalTup]() { stdOptionalTup.reset(); };

        jsonArray->setReferencedValueHandlers(reinitializer, resetter);

        return jsonArray;
    }
};


template<typename T>
std::shared_ptr<JsonValue> 
createJsonPrimitiveValue(T& value) {
    static_assert(is_json_serializable_primitive_type_v<T>);

    return JsonValueCreator<
                                JsonSourceType::Primitive,
                                wrapper_type_v<T>,
                                std::is_const_v<T>
                            >
                            ::create(value);

}

template<typename T>
std::shared_ptr<JsonValue> 
createJsonObject(T& value) {
    static_assert(is_describable_struct_v<T>);

    return JsonValueCreator<
                                JsonSourceType::Struct, 
                                wrapper_type_v<T>, 
                                std::is_const_v<T>
                            >
                            ::create(value);
}

template<typename T>
std::shared_ptr<JsonValue> 
createJsonArrayFromSeq(T& sequence) {
    static_assert(is_json_serializable_sequential_container_v<T>);

    return JsonValueCreator<
                               JsonSourceType::Sequential, 
                               wrapper_type_v<T>, 
                               std::is_const_v<T>
                            >
                            ::create(sequence);
}

template<typename T>
std::shared_ptr<JsonValue> 
createJsonArrayFromTup(T& tuple) {

    static_assert(is_json_serializable_tuple_v<T>);

    return JsonValueCreator<
                                JsonSourceType::Tuple,
                                wrapper_type_v<T>,
                                std::is_const_v<T>
                           >
                           ::create(tuple);
}


template<typename T>
std::shared_ptr<JsonValue> 
convertToJsonValue(T& memberRef) {

    if constexpr (is_json_serializable_primitive_type_v<T>)
        return createJsonPrimitiveValue(memberRef);

    else if constexpr (is_describable_struct_v<T>)
        return createJsonObject(memberRef);

    else if constexpr (is_json_serializable_tuple_v<T>)
        return createJsonArrayFromTup(memberRef);

    else if constexpr (is_json_serializable_sequential_container_v<T>)
        return createJsonArrayFromSeq(memberRef);

    else 
        static_assert(false, "Unsupported type for JSON serialization");
}


template<typename Desc>
std::string getMemberName(Desc descriptor) {
    return std::string(descriptor.name());
}

template<typename Struct, typename Desc>
auto& getMemberValueRef(Struct& s, Desc descriptor) {
    return s.*(descriptor.pointer());
}


template<typename Struct>
std::vector<JsonAttribute> buildJsonTreeFrom(Struct& s) {
    static_assert(is_describable_struct_v<Struct>, "Use the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro to declare serializable struct members");

    std::vector<JsonAttribute> members;

    auto descriptors = Descriptor<std::remove_const_t<Struct>>::member_descriptors;

    for_each(descriptors, 
            [&s, &members](auto desc) {
             std::string name = getMemberName(desc);
             auto& valueRef = getMemberValueRef(s, desc);
             
             members.push_back(JsonAttribute{name, convertToJsonValue(valueRef)});
    });

    return members;
}


template<typename Struct>
std::string marshalImpl(const Struct& s) {
    JsonObject root(buildJsonTreeFrom(s));

    JsonWriter writer;
    return writer.witeToJson(&root);
}

template<typename Struct>
void unmarshalImpl(std::string_view json, Struct& s)  {
    JsonReader reader(json);

    JsonObject root(buildJsonTreeFrom(s));
    reader.readFromJson(&root);
}


}  // namespace detail
}  // namespace rapidjson_util 
           
        
#endif