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
    static_assert(is_jsonable_sequential_container_v<Sequence>);

    std::vector<std::shared_ptr<JsonValue>> elements;

    for (auto&& item : sequence)
        elements.push_back(convertToJsonValue(item));
        
    return elements;
}

template<typename Tuple>
std::vector <std::shared_ptr<JsonValue>> 
tupleToJsonArrayElems(Tuple& tuple) {
    static_assert(is_jsonable_tuple_v<Tuple>);

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
    size_t WrapperType
>
struct JsonValueCreator;

template
<
    size_t UnusedWrapperType
>
struct JsonValueCreator
    <
     JsonSourceType::Primitive,
     UnusedWrapperType
    > {
    
    template<typename T>
    static std::shared_ptr<JsonPrimitiveValue> 
    create(T& value) {
        static_assert(is_jsonable_primitive_type_v<T>);

        return std::make_shared<JsonPrimitiveValue>(&value);
    }
};


template<>
struct JsonValueCreator
    <
    JsonSourceType::Struct, 
    WrapperType::None
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
    WrapperType::StdOptional
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableObject> 
    create(T& optionalStruct) {
        static_assert(is_std_optional_v<T>);

        auto object = optionalStructToNullableObject(optionalStruct);
            
        if constexpr (!std::is_const_v<T>)
            attachOptionalReinitHandlers(object, optionalStruct);

        return object;
    }


private:
    template<typename T>
    static std::shared_ptr<JsonNullableObject>
    optionalStructToNullableObject(T& optionalStruct) {
        if (!optionalStruct.has_value())
            return std::make_shared<JsonNullableObject>();
    
    
        auto attributes = std::is_const_v<T> ?
            buildJsonTreeFrom(std::as_const(optionalStruct.value())) :
            buildJsonTreeFrom(optionalStruct.value());
    
        return std::make_shared<JsonNullableObject>(attributes);
    }
        
        
    template<typename T>
    static void 
    attachOptionalReinitHandlers(
        std::shared_ptr<JsonNullableObject> object, T& optionalStruct) {
        
        auto resetter = [&optionalStruct]() { optionalStruct.reset(); };
        auto reinitializer = [&optionalStruct]() {
            using BaseType = remove_std_optional_t<T>;
        
            // Initialize the std::optional<Struct> variable from std::null_opt
            optionalStruct = BaseType{};
        
            // After initialization, update the subtree of its nested member pointers recursively
            auto object = JsonValueCreator<
                JsonSourceType::Struct,
                WrapperType::StdOptional
            >::create(optionalStruct);
        
            return object->getMembers();
        };
        
        object->setReferencedValueHandlers(reinitializer, resetter);
    }
};


template<>
struct JsonValueCreator
    <
    JsonSourceType::Sequential, 
    WrapperType::None
    > {

    template<typename T>
    static std::shared_ptr<JsonArray> 
    create(T& sequence) {
        static_assert(!is_std_optional_v<T>);

        auto elements = seqToJsonArrayElems(sequence);
        bool containOptElems = contain_std_optional_elements<T>::value;
        auto jsonArray = std::make_shared<JsonArray>(elements, containOptElems);

        if constexpr (!std::is_const_v<T> && is_jsonable_dynamic_array_v<T>)
            attachArrayResizer(jsonArray, sequence);

        return jsonArray;
    }

private:
    template<typename T>
    static void
    attachArrayResizer(std::shared_ptr<JsonArray> jsonArray, T& sequence) {
        auto resizer = [&sequence](std::size_t newSize) {
            sequence.resize(newSize);
            return seqToJsonArrayElems(sequence);
        };

        jsonArray->setArrayResizer(resizer);
    }
};


template<>
struct JsonValueCreator
    <
    JsonSourceType::Sequential, 
    WrapperType::StdOptional
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableArray> 
    create(T& optionalSeq) {
        static_assert(is_std_optional_v<T>);

        auto nullableArray = optionalSeqToNullableArray(optionalSeq);

        if constexpr (!std::is_const_v<T>) {
            if constexpr (is_jsonable_dynamic_array_v<T>)
                attachArrayResizer(nullableArray, optionalSeq);

            attachOptionalReinitHandlers(nullableArray, optionalSeq);
        }
      
        return nullableArray;
    }


private:
    template<typename T>
    static std::shared_ptr<JsonNullableArray>
    optionalSeqToNullableArray(T& optionalSeq) {

        bool containOpt = contain_std_optional_elements<T>::value;
        if (!optionalSeq.has_value())
            return std::make_shared<JsonNullableArray>(containOpt);


        auto elements = std::is_const_v<T> ?
            seqToJsonArrayElems(std::as_const(optionalSeq.value())) : 
            seqToJsonArrayElems(optionalSeq.value());

        return std::make_shared<JsonNullableArray>(elements, containOpt);
    }

    template<typename T>
    static void
    attachArrayResizer(std::shared_ptr<JsonNullableArray> nullableArray, T& optionalSeq) {
        auto resizer = [&optionalSeq](std::size_t newSize) {
            optionalSeq->resize(newSize);
            return seqToJsonArrayElems(optionalSeq.value());
        };

        nullableArray->setArrayResizer(resizer);
    }

    template<typename T>
    static void
    attachOptionalReinitHandlers(
    std::shared_ptr<JsonNullableArray> nullableArray, T& optionalSeq) {
        auto reinitializer = [&optionalSeq]() {
            using BaseType = remove_std_optional_t<T>;
            optionalSeq = BaseType{};
            return seqToJsonArrayElems(optionalSeq.value());
        };

        auto resetter = [&optionalSeq]() { optionalSeq.reset(); };


        nullableArray->setReferencedValueHandlers(reinitializer, resetter);
    }

};


template<>
struct JsonValueCreator
    <
    JsonSourceType::Tuple, 
    WrapperType::None
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
    WrapperType::StdOptional
    > {

    template<typename T>
    static std::shared_ptr<JsonNullableArray> 
    create(T& optionalSeq) {
        static_assert(is_std_optional_v<T>);

        auto nullableArray = optionalTupToNullableArray(optionalSeq); 

        if constexpr (!std::is_const_v<T>) 
            attachOptionalReinitHandlers(nullableArray, optionalSeq);

        return nullableArray;
    }


private:
    template<typename T>
    static std::shared_ptr<JsonNullableArray> 
    optionalTupToNullableArray(T& optionalTup) {
        if(!optionalTup.has_value())
            return std::make_shared<JsonNullableArray>();


        auto elements = std::is_const_v<T> ?
            tupleToJsonArrayElems(std::as_const(optionalTup.value())) :
            tupleToJsonArrayElems(optionalTup.value());

        return std::make_shared<JsonNullableArray>(elements);
    }


    template<typename T>
    static void
    attachOptionalReinitHandlers(
    std::shared_ptr<JsonNullableArray> nullableArray, T& optionalSeq) {
        auto reinitializer = [&optionalSeq]() {
            using BaseType = remove_std_optional_t<T>;
        
            // Initialize the std::optional<std::tuple<...>> variable from std::null_opt
            optionalSeq = BaseType{};
        
            // After initialization, update the subtree of its nested member pointers recursively
            return tupleToJsonArrayElems(optionalSeq.value());
        };
        
        auto resetter = [&optionalSeq]() { optionalSeq.reset(); };
        

        nullableArray->setReferencedValueHandlers(reinitializer, resetter);
    }
};


template<typename T>
using FromPrimitive = JsonValueCreator<JsonSourceType::Primitive, wrapper_type_v<T>>;

template<typename T>
using FromStruct = JsonValueCreator<JsonSourceType::Struct, wrapper_type_v<T>>;

template<typename T>
using FromTuple = JsonValueCreator<JsonSourceType::Tuple, wrapper_type_v<T>>;

template<typename T>
using FromSequence = JsonValueCreator<JsonSourceType::Sequential, wrapper_type_v<T>>;


template<typename T>
std::shared_ptr<JsonValue> 
convertToJsonValue(T& memberRef) {
    if constexpr (is_jsonable_primitive_type_v<T>)
        return FromPrimitive<T>::create(memberRef);

    else if constexpr (is_describable_struct_v<T>)
        return FromStruct<T>::create(memberRef);

    else if constexpr (is_jsonable_tuple_v<T>)
        return FromTuple<T>::create(memberRef);

    else if constexpr (is_jsonable_sequential_container_v<T>)
        return FromSequence<T>::create(memberRef);

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