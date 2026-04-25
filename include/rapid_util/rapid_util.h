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
    return detail::marshalImpl(s);
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
    return detail::unmarshalImpl(json, s);
}

namespace detail {


template<typename T>
std::shared_ptr<JsonValue> convertToJsonValueFrom(T& memberRef);

template<typename Sequence>
std::vector<std::shared_ptr<JsonValue>> convertSequenceToJsonArrayElements(Sequence& sequence) {
    static_assert(is_json_serializable_sequential_container_v<Sequence>);

    std::vector<std::shared_ptr<JsonValue>> elements;

    for (auto&& item : sequence)
        elements.push_back(convertToJsonValueFrom(item));
        
    return elements;
}

template<typename Tuple>
std::vector <std::shared_ptr<JsonValue>> convertTupleToJsonArrayElements(Tuple& tuple) {
    static_assert(is_json_serializable_tuple_v<Tuple>);

    std::vector<std::shared_ptr<JsonValue>> elements;

    std::apply([&elements](auto&&... tupleArgs) {
                           (..., (elements.push_back(convertToJsonValueFrom(tupleArgs))));}, tuple);

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
constexpr WrapperType wrapper_type_trait_v = is_std_optional_v<T> ? WrapperType::StdOptional : WrapperType::None;

template<size_t JsonSourceType, size_t WrapperType, bool isConstQualified>
struct JsonValueCreator;

template<size_t UnusedWrapperType, bool UnusedIsConstQualified>
struct JsonValueCreator<JsonSourceType::Primitive, UnusedWrapperType, UnusedIsConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonPrimitiveValue> create(T& value) {
        static_assert(is_json_serializable_primitive_type_v<T>);

        return std::make_shared<JsonPrimitiveValue>(&value);
    }
};


template<bool unusedIsConstQualified>
struct JsonValueCreator<JsonSourceType::Struct, WrapperType::None, unusedIsConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonObject> create(T& value) {
        static_assert(!is_std_optional_v<T>);

        return std::make_shared<JsonObject>(buildJsonTreeFrom(value));
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Struct, WrapperType::StdOptional, true> {
    template<typename T>
    static std::shared_ptr<JsonNullableObject> create(T& stdOptionalStruct) {
        static_assert(is_std_optional_v<T> && std::is_const_v<T>);

        if (stdOptionalStruct.has_value())
            return std::make_shared<JsonNullableObject>(buildJsonTreeFrom(std::as_const(stdOptionalStruct.value())));
        else
            return std::make_shared<JsonNullableObject>();
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Struct, WrapperType::StdOptional, false> {
    template<typename T>
    static std::shared_ptr<JsonNullableObject> create(T& stdOptionalStruct) {
        static_assert(is_std_optional_v<T> && !std::is_const_v<T>);

        auto object = (stdOptionalStruct.has_value()) ? std::make_shared<JsonNullableObject>(buildJsonTreeFrom(stdOptionalStruct.value())) :
                                                        std::make_shared<JsonNullableObject>();
                                           
        auto referencedValueResetter = [&stdOptionalStruct]() { stdOptionalStruct.reset(); };
        auto referencedValueReinitializer = [&stdOptionalStruct]() {
                                                    using BaseType = remove_std_optional_t<T>;

                                                    // Initialize the std::optional<Struct> variable from std::null_opt
                                                    stdOptionalStruct = BaseType{}; 
                                                
                                                    // After initialization, update the subtree of its nested member pointers recursively
                                                    auto object = JsonValueCreator<JsonSourceType::Struct, 
                                                                                   WrapperType::StdOptional, 
                                                                                   false>::create(stdOptionalStruct);
                                                    return object->getMembers();
                                                };

        object->setReferencedValueHandlers(referencedValueReinitializer, referencedValueResetter);

        return object;
    }
};


template<bool isConstQualified>
struct JsonValueCreator<JsonSourceType::Sequential, WrapperType::None, isConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonArray> create(T& sequence) {
        static_assert(!is_std_optional_v<T>);

        auto elements = convertSequenceToJsonArrayElements(sequence);
        auto jsonArray = std::make_shared<JsonArray>(elements, contain_std_optional_elements<T>::value);

        if constexpr(!isConstQualified && is_json_serializable_dynamic_array_v<T>)
            jsonArray->setArrayResizer(
                                  [&sequence](std::size_t newSize) {
                                            sequence.resize(newSize);
                                            return  convertSequenceToJsonArrayElements(sequence);
                                       });
            
        return jsonArray;
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Sequential, WrapperType::StdOptional, true> {
    template<typename T>
    static std::shared_ptr<JsonNullableArray> create(T& sequence) {
        static_assert(is_std_optional_v<T> && std::is_const_v<T>);

        bool containOptionalElems = contain_std_optional_elements<T>::value;

        if (sequence.has_value()) {
            auto elements = convertSequenceToJsonArrayElements(std::as_const(sequence.value()));
            return std::make_shared<JsonNullableArray>(elements, containOptionalElems);
        }

        else
            return std::make_shared<JsonNullableArray>(containOptionalElems);
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Sequential, WrapperType::StdOptional, false> {
    template<typename T>
    static std::shared_ptr<JsonNullableArray> create(T& sequence) {
        static_assert(is_std_optional_v<T> &&  !std::is_const_v<T>);

        bool containOptionalElems = contain_std_optional_elements<T>::value;
        auto jsonArray = (sequence.has_value()) ?
                            std::make_shared<JsonNullableArray>(convertSequenceToJsonArrayElements(sequence.value()), containOptionalElems) :
                            std::make_shared<JsonNullableArray>(containOptionalElems);
                                
        auto optValueReinitializer = [&sequence]() {
                                          using BaseType = remove_std_optional_t<T>;
                                          sequence = BaseType{};
                                          
                                          return std::vector<std::shared_ptr<JsonValue>>{};
                                     };

        auto resizer = [&sequence, optValueReinitializer]
                       (std::size_t newSize) {
                            if (!sequence.has_value())
                                optValueReinitializer();
        
                            sequence->resize(newSize);
                            return  convertSequenceToJsonArrayElements(sequence.value());
                        };

        auto optValueResetter = [&sequence]() { sequence.reset(); };
        
        jsonArray->setArrayResizer(resizer);
        jsonArray->setReferencedValueHandlers(optValueReinitializer, optValueResetter);
        
        return jsonArray;
    }
};


template<bool UnusedIsConstQualified>
struct JsonValueCreator<JsonSourceType::Tuple, WrapperType::None, UnusedIsConstQualified> {
    template<typename T>
    static std::shared_ptr<JsonArray> create(T& tuple) {
        static_assert(!is_std_optional_v<T>);

        auto elements = convertTupleToJsonArrayElements(tuple);

        return  std::make_shared<JsonArray>(elements);
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Tuple, WrapperType::StdOptional, true> {
    template<typename T>
    static std::shared_ptr<JsonNullableArray> create(T& stdOptionalTup) {
        static_assert(is_std_optional_v<T> && std::is_const_v<T>);

        if (stdOptionalTup.has_value()) {
            auto elements = convertTupleToJsonArrayElements(std::as_const(stdOptionalTup.value()));
            return std::make_shared<JsonNullableArray>(elements);
        }
            
        else
            return std::make_shared<JsonNullableArray>();
    }
};

template<>
struct JsonValueCreator<JsonSourceType::Tuple, WrapperType::StdOptional, false> {
    template<typename T>
    static std::shared_ptr<JsonNullableArray> create(T& stdOptionalTup) {
        static_assert(is_std_optional_v<T> && !std::is_const_v<T>);

        auto jsonArray = stdOptionalTup.has_value() ?
                            std::make_shared<JsonNullableArray>(convertTupleToJsonArrayElements(stdOptionalTup.value())) :
                            std::make_shared<JsonNullableArray>();
                                              
        auto referencedValueReinitializer = [&stdOptionalTup]() {
                               using BaseType = remove_std_optional_t<T>;

                               // Initialize the std::optional<std::tuple<...>> variable from std::null_opt
                               stdOptionalTup = BaseType{};
                               
                               // After initialization, update the subtree of its nested member pointers recursively
                               return convertTupleToJsonArrayElements(stdOptionalTup.value());
                            };

        auto referencedValueResetter = [&stdOptionalTup]() { stdOptionalTup.reset(); };

        jsonArray->setReferencedValueHandlers(referencedValueReinitializer, referencedValueResetter);

        return jsonArray;
    }
};


template<typename T>
std::shared_ptr<JsonValue> createJsonPrimitiveValueFrom(T& value) {
    static_assert(is_json_serializable_primitive_type_v<T>);

    return JsonValueCreator<JsonSourceType::Primitive, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(value);
}

template<typename T>
std::shared_ptr<JsonValue> createJsonObjectFrom(T& value) {
    static_assert(is_describable_struct_v<T>);

    return JsonValueCreator<JsonSourceType::Struct, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(value);
}

template<typename T>
std::shared_ptr<JsonValue> createJsonArrayFromSeq(T& sequence) {
    static_assert(is_json_serializable_sequential_container_v<T>);

    return JsonValueCreator<JsonSourceType::Sequential, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(sequence);
}

template<typename T>
std::shared_ptr<JsonValue> createJsonArrayFromTup(T& tuple) {
    static_assert(is_json_serializable_tuple_v<T>);

    return JsonValueCreator<JsonSourceType::Tuple, wrapper_type_trait_v<T>, std::is_const_v<T>>::create(tuple);
}


template<typename T>
std::shared_ptr<JsonValue> convertToJsonValueFrom(T& memberRef) {
    if constexpr (is_json_serializable_primitive_type_v<T>)
        return createJsonPrimitiveValueFrom(memberRef);

    else if constexpr (is_describable_struct_v<T>)
        return createJsonObjectFrom(memberRef);

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
    for_each(descriptors, [&s, &members](auto desc) {
                              std::string name = getMemberName(desc);
                              auto& valueRef = getMemberValueRef(s, desc);
                              
                              members.push_back(JsonAttribute{ name, convertToJsonValueFrom(valueRef) });
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


#define RAPIDJSON_UTIL_CHECK_MEMBERS_ARE_SERIALIZABLE(C, members) \
        RAPIDJSON_UTIL_STRIP_COMMAS(RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE, C, RAPIDJSON_UTIL_UNPACK members))

#define RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE(C, member) \
        static_assert(rapidjson_util::detail::is_json_serializable_v<rapidjson_util::detail::member_type_t<decltype(&C::member)>>, "Member variable types must be compatible with JSON value types.");


#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMP(C, members)  template<> struct rapidjson_util::detail::Descriptor<C> {  \
     	static constexpr bool is_describable = true;                                                                \
        static constexpr auto member_descriptors = make_typelist(                                                   \
                       RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_MEMBER_META, C, RAPIDJSON_UTIL_UNPACK members));      \
        };


#define RAPIDJSON_UTIL_MEMBER_META(C, member)                                                    \
	[]{ struct rapidjsonUtilDesc {                                                               \
             static constexpr auto pointer() noexcept { return &C::member; }                     \
             static constexpr auto name() noexcept { return RAPIDJSON_UTIL_STRINGIFY(member); }  \
    }; return rapidjsonUtilDesc{}; }  () 

}  // namespace detail
}  // namespace rapidjson_util 

#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS(C, members)                \
        static_assert(std::is_class_v<C>);                         \
        RAPIDJSON_UTIL_CHECK_MEMBERS_ARE_SERIALIZABLE(C, members)  \
        RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMP(C, members)                 
        
#endif