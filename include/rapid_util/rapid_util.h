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


#ifndef __SIMPLE_RAPID_JSON_UTIL_H__
#define __SIMPLE_RAPID_JSON_UTIL_H__

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>
#include <stack>
#include <stdexcept>
#include <string_view>
#include "rapid_util_preprocessor.h"
#include "rapid_util_value.h"

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


/**
 * @brief Exception thrown when a JSON object member fails to deserialize
 */
class SerializationError : public std::runtime_error {
public:
    SerializationError(std::string_view what) 
        : runtime_error(std::string(what)) { }
};


/**
 * @brief Exception thrown when required JSON member is missing
 */
class MemberNotFoundError : public SerializationError {
public:
    MemberNotFoundError(std::string_view member)
        : SerializationError(
            std::string("JSON doesn't match the struct -- required field \"")
            .append(member)
            .append("\" not found")) { }
};


/**
 * @brief Exception thrown when JSON value type doesn't match the expected type of structs
 */
class TypeMismatchError : public SerializationError {
public:
    TypeMismatchError(std::string_view what)
        : SerializationError(what.data()) { }
};


/**
 * @brief Exception thrown when JSON array size doesn't match the array size of structs
 */
class ArrayLengthMismatchError : public SerializationError {
public:
    ArrayLengthMismatchError(std::string_view what)
        : SerializationError(what.data()) { }
};


/**
 * @brief Exception thrown when the JSON input has invalid syntax
 */
class InvalidJsonError : public std::logic_error {
public:
    InvalidJsonError(std::string_view what) 
        : std::logic_error(std::string(what)) { }
};


/**
 * @brief Exception thrown when attempting to parse an empty JSON string
 */
class EmptyJsonStringError : public std::logic_error {
public:
    EmptyJsonStringError() 
        : std::logic_error("The JSON string to be parsed is empty") { }
};


namespace detail {


template<typename Exception>
void THROW_EXCEPTION_IF(bool condition, Exception&& exception) {
    if (condition)
        throw std::forward<Exception>(exception);
}


enum class QueryType {
    IsInt,
    IsUint,
    IsInt64,
    IsUint64,
    IsFloat,
    IsDouble,
    IsBool,
    IsString,
    IsObject,
    IsArray
};


class TypeValidator {
public:
    static void validate(const rapidjson::Value& value, QueryType type) {
#define RAPIDJSON_VALUE_VALIDATE(value, query, expectedType)												        \
		    if(!value. ## query())																					\
			    throw TypeMismatchError(std::string("Expected ") + expectedType + ", got " + getTypeFrom(value));

        switch (type) {
        case QueryType::IsInt:    RAPIDJSON_VALUE_VALIDATE(value, IsInt,    "Int");    break;
        case QueryType::IsUint:   RAPIDJSON_VALUE_VALIDATE(value, IsUint,   "Uint");   break;
        case QueryType::IsInt64:  RAPIDJSON_VALUE_VALIDATE(value, IsInt64,  "Int64");  break;
        case QueryType::IsUint64: RAPIDJSON_VALUE_VALIDATE(value, IsUint64, "Uint64"); break;
        case QueryType::IsFloat:  RAPIDJSON_VALUE_VALIDATE(value, IsFloat,  "Float");  break;
        case QueryType::IsDouble: RAPIDJSON_VALUE_VALIDATE(value, IsDouble, "Double"); break;
        case QueryType::IsBool:   RAPIDJSON_VALUE_VALIDATE(value, IsBool,   "Bool");   break;
        case QueryType::IsString: RAPIDJSON_VALUE_VALIDATE(value, IsString, "String"); break;
        case QueryType::IsArray:  RAPIDJSON_VALUE_VALIDATE(value, IsArray,  "Array");  break;
        case QueryType::IsObject: RAPIDJSON_VALUE_VALIDATE(value, IsObject, "Object"); break;
        }

#undef RAPIDJSON_VALUE_VALIDATE
    }

private:
    static std::string getTypeFrom(const rapidjson::Value& value) {
        if (value.IsNumber()) {
            if (value.IsInt())    return "Int";
            if (value.IsUint())   return "Uint";
            if (value.IsInt64())  return "Int64";
            if (value.IsUint64()) return "Uint64";
            if (value.IsDouble()) return "Double";
            if (value.IsFloat())  return "Float";
            return "Number";
        }

        if (value.IsNull())    return "Null";
        if (value.IsBool())    return "Boolean";
        if (value.IsObject())  return "Object";
        if (value.IsArray())   return "Array";
        if (value.IsString())  return "String";
        return "Unknown";
    }
};


using SizeType = unsigned;

class JsonReader {
public:
    /**
      * @brief Construct a JsonReader with JSON input for parsing
      *
      * @param jsonInput JSON string to parse and deserialize
      */
    JsonReader(std::string_view jsonInput) {
        if (jsonInput.empty())
            throw EmptyJsonStringError{};

        if (doc.Parse(jsonInput.data()).HasParseError())
            throw InvalidJsonError("The provided JSON text has invalid syntax");
    }

    void readFromJson(Value& root) {
        assert(root.isObject() && root.isModifiable());

        read(root, doc);
    }

private:
    void read(Value& v, const rapidjson::Value& json) {
        if (v.canBeNull() && json.IsNull()) 
            return v.setNull();
  
  
        if (v.isPrimitive()) {
            auto p = v.asPrimitive();
            read(p, json);
        }
        else if (v.isObject()) {
            auto o = v.asObject();
            read(o, json);
        }
        else if (v.isArray()) {
            auto a = v.asArray();
            read(a, json);
        }
        else
            assert(false);
    }

    void read(Value::PrimitiveType& p, const rapidjson::Value& json) {
        if (p.isBool())        { validateAndPrepare(json, QueryType::IsBool,   p); p.setBool(json.GetBool()); }
        else if (p.isInt())    { validateAndPrepare(json, QueryType::IsInt,    p); p.setInt(json.GetInt()); }
        else if (p.isUint())   { validateAndPrepare(json, QueryType::IsUint,   p); p.setUint(json.GetUint()); }
        else if (p.isInt64())  { validateAndPrepare(json, QueryType::IsInt64,  p); p.setInt64(json.GetInt64()); }
        else if (p.isUint64()) { validateAndPrepare(json, QueryType::IsUint64, p); p.setUint64(json.GetUint64()); }
        else if (p.isFloat())  { validateAndPrepare(json, QueryType::IsFloat,  p); p.setFloat(json.GetFloat()); }
        else if (p.isDouble()) { validateAndPrepare(json, QueryType::IsDouble, p); p.setDouble(json.GetDouble()); }
        else if (p.isString()) { validateAndPrepare(json, QueryType::IsString, p); p.setString(json.GetString()); }
        else assert(false);
    }

    void read(Value::ObjectType& object, const rapidjson::Value& json) {
        validateAndPrepare(json, QueryType::IsObject, object);

        for (auto& member : object) {
            const auto name = member.name().c_str();
            auto& value = member.value();

            THROW_EXCEPTION_IF(!json.HasMember(name),
                                MemberNotFoundError(name));

            try {
                read(value, json[name]);
            }
            catch (SerializationError& e) {
                throw SerializationError(std::string("Deserialization of member \"") +
                    name + "\" failed: " + e.what());
            }
        }
    }

    void read(Value::ArrayType& array, const rapidjson::Value& json) {
        validateAndPrepare(json, QueryType::IsArray, array);

        if (containNullElements(json))
            THROW_EXCEPTION_IF(!array.canHoldNullElem(),
                               TypeMismatchError("JSON array contains null elements"));

        THROW_EXCEPTION_IF(json.Size() != array.size() && !array.isResizable(),
                               ArrayLengthMismatchError(
                                   "Array size mismatch: JSON contains " + std::to_string(json.Size()) +
                                   " elements, but given array has fixed capacity of " + std::to_string(array.size()) +
                                   " elements and cannot be resized."));


        if (json.Size() != array.size())
            array.resize(json.Size());

        SizeType i = 0;
        for (auto& elem : array) 
            read(elem, json[i++]);
    }

    void validateAndPrepare(const rapidjson::Value& json, QueryType type, Value& v) {
        TypeValidator::validate(json, type);

        if (v.isNull())
            v.reinit();
    }

    bool containNullElements(const rapidjson::Value& value) {
        assert(value.IsArray());

        for (auto&& elem : value.GetArray())
            if (elem.IsNull())
                return true;

        return false;
    }

    rapidjson::Document doc;
};


template<typename Handler>
class JsonWriter : private Handler {
public:
    using Handler::Handler;

    void writeToJson(const Value& root) {
        assert(root.isObject());

        auto obj = root.asConstObject();
        write(obj);
    }

private:
    bool Float(float f) {
        return Double(static_cast<double>(f));
    }

    bool String(std::string_view str) {
        auto size = static_cast<SizeType>(str.size());

        return Handler::String(str.data(), size);
    }

    void write(const Value& v) {
        if (v.isNull()) {
            Null();
        }
        else if (v.isPrimitive()) {
            auto p = v.asConstPrimitive();
            write(p);
        }
        else if (v.isObject()) {
            auto o = v.asConstObject();
            write(o);
        }
        else if (v.isArray()) {
            auto a = v.asConstArray();
            write(a);
        }
        else 
            assert(false);
        
    }

    void write(Value::ConstPrimitiveType& p) {
        if (p.isBool())        Bool(p.getBool());
        else if (p.isInt())    Int(p.getInt());
        else if (p.isUint())   Uint(p.getUint());
        else if (p.isInt64())  Int64(p.getInt64());
        else if (p.isUint64()) Uint64(p.getUint64());
        else if (p.isFloat())  Float(p.getFloat());
        else if (p.isDouble()) Double(p.getDouble());
        else if (p.isString()) String(p.getString());
        else assert(false);
    }

    void write(Value::ConstObjectType& object) {
        StartObject();

        for (auto& member : object) {
            const auto& name = member.name();
            Key(name.c_str(), static_cast<SizeType>(name.size()));

            write(member.value());
        }

        EndObject();
    }

    void write(Value::ConstArrayType& array) {
        StartArray();

        for (auto& elem : array) 
            write(elem);
        
        EndArray();
    }
};


template<typename Struct>
std::string marshalImpl(const Struct& s) {
    static_assert(is_jsonable_struct_v<Struct>,
        "Use the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro to declare serializable struct members");

    Value root(&s);

    rapidjson::StringBuffer buffer;
    JsonWriter<rapidjson::Writer<rapidjson::StringBuffer>> writer(buffer);

    writer.writeToJson(root);

    return buffer.GetString();
}

template<typename Struct>
void unmarshalImpl(std::string_view json, Struct& s)  {
    static_assert(is_jsonable_struct_v<Struct>,
        "Use the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro to declare serializable struct members");

    JsonReader reader(json);

    Value root(&s);
    reader.readFromJson(root);
}


}  // namespace detail

}  // namespace rapidjson_util 
           
        
#endif