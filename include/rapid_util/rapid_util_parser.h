#ifndef __RAPID_UTIL_PARSER_H__
#define __RAPID_UTIL_PARSER_H__

#include "rapid_util_preprocessor.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string_view>
#include <string>
#include <memory>
#include <functional>
#include <cstdint>
#include <any>
#include <stdexcept>

namespace rapidjson_util {

/**
 * @brief Exception thrown when a JSON object member fails to deserialize
 */
class MemberSerializationFailure : public std::logic_error {
public:
	MemberSerializationFailure(std::string_view what);
};


/**
 * @brief Exception thrown when required JSON member is missing
 */
class MemberNotFoundException : public std::logic_error {
public:
	MemberNotFoundException(std::string_view member);
};


/**
 * @brief Exception thrown when JSON value type doesn't match the expected type of structs
 */
class TypeMismatchException : public std::logic_error {
public:
	TypeMismatchException(std::string_view what);
};


/**
 * @brief Exception thrown when JSON array size doesn't match the array size of structs
 */
class ArrayLengthMismatchException : public std::logic_error {
public:
	ArrayLengthMismatchException(std::string_view what);
};


/**
 * @brief Exception thrown when the JSON input has invalid syntax
 */
class InvalidJsonException : public std::logic_error {
public:
	InvalidJsonException(std::string_view what);
};


/**
 * @brief Exception thrown when attempting to parse an empty JSON string
 */
class EmptyJsonStringException : public std::logic_error {
public:
	EmptyJsonStringException();
};


namespace detail {

class JsonBasicValue;
class JsonObject;
class JsonArray;

template<typename Exception>
void throwIfFalse(bool condition, Exception&& exception) {
	if (!condition)
		throw std::forward<Exception>(exception);
}


/**
 * @brief Visitor interface for JSON value types using the Visitor pattern
 *
 * Implemented by JsonReader and JsonWriter to handle serialization and
 * deserialization.
 */
class JsonVisitor {
public:
	virtual void visit(JsonBasicValue*, rapidjson::Value& rapidjsonValue) = 0;
	virtual void visit(JsonObject*, rapidjson::Value& rapidjsonValue) = 0;
	virtual void visit(JsonArray*, rapidjson::Value& rapidjsonValue) = 0;
};


/**
 * @brief Serializes C++ struct objects to JSON strings using RapidJSON library
 *
 * Implements the JsonVisitor interface to traverse the JSON object hierarchy
 * derived from struct definitions and populate RapidJSON document structures
 * for serialization.
 */
class JsonWriter : public JsonVisitor {
public:

	/**
      * @brief Serialize a JSON object hierarchy to JSON string 
      *
      * The function traverses the object hierarchy starting from the root node
      * and generates a formatted JSON string from it.
      *
	  * @param root Root node of a pre-populated JSON object hierarchy derived from
	  *             C++ structs tagged by the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro.
      * @return JSON string representation
      */
	std::string witeToJson(JsonObject* root);

	void visit(JsonBasicValue* basicValue, rapidjson::Value& rapidjsonValue) override;
	void visit(JsonObject* object, rapidjson::Value& rapidjsonValue) override;
	void visit(JsonArray* array, rapidjson::Value& rapidjsonValue) override;

private:
	rapidjson::Document rapidjsonDocument;
};


/**
 * @brief Deserializes JSON strings to C++ struct objects
 *
 */
class JsonReader : public JsonVisitor {
public:

	/**
      * @brief Construct a JsonReader with JSON input for parsing
      *
      * @param jsonInput JSON string to parse and deserialize
      */
	JsonReader(std::string_view jsonInput);

	/**
      * @brief Deserializes JSON input provided during construction and updates corresponding 
	  *        C++ struct members
	  *
      * @param root The root node of a JSON object hierarchy derived from C++ structs tagged by
      *             the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro. This JSON object hierarchy contains
      *             pointers to C++ struct member variables, and modifying through these pointers
      *             updates the C++ struct members' state.
	  *
	  */
	void readFromJson(JsonObject* root);
	 
	void visit(JsonBasicValue* basicValue, rapidjson::Value& rapidjsonValue) override;
	void visit(JsonObject* object, rapidjson::Value& rapidjsonValue) override;
	void visit(JsonArray* array, rapidjson::Value& rapidjsonValue) override;

private:
	rapidjson::Document rapidjsonDocument;
};


/**
 * @brief Base class for all JSON-serializable value types
 *
 * Implements the Accept method for the Visitor pattern, allowing
 * JsonReader and JsonWriter to process different value types uniformly.
 */
class JsonValue {
public:
	virtual void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) = 0;
};


class JsonBasicValue : public JsonValue {
public:
	enum class StoredType {
		IntPtr,
		Int64Ptr,
		Uint64Ptr,
		FloatPtr,
		DoublePtr,
		BoolPtr,
		StringPtr
	};

	template<typename T>
	JsonBasicValue(T* _value) : value(_value) {
		using BaseType = std::remove_const_t<T>;

		if constexpr (std::is_same_v<BaseType, int>) type = StoredType::IntPtr;
		else if constexpr (std::is_same_v<BaseType, int64_t>) type = StoredType::Int64Ptr;
		else if constexpr (std::is_same_v<BaseType, uint64_t>) type = StoredType::Uint64Ptr;
		else if constexpr (std::is_same_v<BaseType, bool>) type = StoredType::BoolPtr;
		else if constexpr (std::is_same_v<BaseType, float>) type = StoredType::FloatPtr;
		else if constexpr (std::is_same_v<BaseType, double>) type = StoredType::DoublePtr;
		else if constexpr (std::is_same_v<BaseType, std::string>) type = StoredType::StringPtr;

		pointToConst = std::is_const_v<T>;
	}

	bool IsPointToConst() const {
		return pointToConst;
	}

	StoredType storedType() const {
		return type;
	}

	std::any storedValue() const {
		return value;
	}

	void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) override {
		visitor.visit(this, rapidjsonValue);
	}

private:
	bool pointToConst;
	std::any value;
	StoredType type;
};


struct JsonAttribute {
	std::string name;
	std::shared_ptr<JsonValue> value;
};


class JsonObject : public JsonValue {
public:
	JsonObject(const std::vector<JsonAttribute>& _members = {}) :
		members(_members) {
	}

	void setMembers(const std::vector<JsonAttribute>& _members) {
		members = _members;
	}

	std::vector<JsonAttribute> getMembers() const {
		return members;
	}

	void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) override {
		visitor.visit(this, rapidjsonValue);
	}

private:
	std::vector<JsonAttribute> members;
};


class JsonArray : public JsonValue {
public:
	using ArrayResizer = std::function <std::vector<std::shared_ptr<JsonValue>>(std::size_t)>;

	JsonArray(const std::vector<std::shared_ptr<JsonValue>>& _elements) :
		elements(_elements) {
	}

	JsonArray(const std::vector<std::shared_ptr<JsonValue>>& _elements, ArrayResizer _resizer) :
		elements(_elements), resizer(_resizer) {
		assert(_resizer != nullptr);
	}

	bool isResizable() const {
		return resizer != nullptr;
	}

	void resize(std::size_t newSize) {
		elements = resizer(newSize);
	}

	std::size_t size() const {
		return elements.size();
	}

	std::vector<std::shared_ptr<JsonValue>> getElements() const {
		return elements;
	}

	void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) override {
		visitor.visit(this, rapidjsonValue);
	}

private:
	ArrayResizer resizer = nullptr;
	std::vector<std::shared_ptr<JsonValue>> elements;
};


inline std::string JsonWriter::witeToJson(JsonObject* root) {
	root->accept(*this, rapidjsonDocument);

	rapidjson::StringBuffer buffer;

	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	rapidjsonDocument.Accept(writer);

	return buffer.GetString();
}

inline void JsonWriter::visit(JsonBasicValue* basicValue, rapidjson::Value& rapidjsonValue) {
	assert(basicValue->IsPointToConst());

	switch (basicValue->storedType()) {
		case JsonBasicValue::StoredType::IntPtr: {
			auto value = std::any_cast<const int*>(basicValue->storedValue());
			rapidjsonValue.SetInt(*value);
			break;
		}

		case JsonBasicValue::StoredType::Int64Ptr: {
			auto value = std::any_cast<const int64_t*>(basicValue->storedValue());
			rapidjsonValue.SetInt64(*value);
			break;
		}

		case JsonBasicValue::StoredType::Uint64Ptr: {
			auto value = std::any_cast<const uint64_t*>(basicValue->storedValue());
			rapidjsonValue.SetUint64(*value);
			break;
		}

		case JsonBasicValue::StoredType::BoolPtr: {
			auto value = std::any_cast<const bool*>(basicValue->storedValue());
			rapidjsonValue.SetBool(*value);
			break;
		}

		case JsonBasicValue::StoredType::FloatPtr: {
			auto value = std::any_cast<const float*>(basicValue->storedValue());
			rapidjsonValue.SetFloat(*value);
			break;
		}

		case JsonBasicValue::StoredType::DoublePtr: {
			auto value = std::any_cast<const double*>(basicValue->storedValue());
			rapidjsonValue.SetDouble(*value);
			break;
		}

		case JsonBasicValue::StoredType::StringPtr: {
			auto value = std::any_cast<const std::string*>(basicValue->storedValue());
			rapidjsonValue.SetString(value->c_str(), static_cast<rapidjson::SizeType>(value->length()),
				                     rapidjsonDocument.GetAllocator());
			break;
		}
	}
}

inline void JsonWriter::visit(JsonObject* object, rapidjson::Value& rapidjsonValue) {
	rapidjsonValue.SetObject();

	for (auto&& member : object->getMembers()) {
		rapidjson::Value name(member.name.c_str(), rapidjsonDocument.GetAllocator());

		rapidjson::Value value;
		member.value->accept(*this, value);

		rapidjsonValue.AddMember(name, value, rapidjsonDocument.GetAllocator());
	}
}

inline void JsonWriter::visit(JsonArray* array, rapidjson::Value& rapidjsonValue) {
	rapidjsonValue.SetArray();

	for (auto&& element : array->getElements()) {
		rapidjson::Value value;
		element->accept(*this, value);

		rapidjsonValue.PushBack(value, rapidjsonDocument.GetAllocator());
	}
}

inline JsonReader::JsonReader(std::string_view json) {
	if (json.empty())
		throw EmptyJsonStringException{};

	if (rapidjsonDocument.Parse(json.data()).HasParseError())
		throw InvalidJsonException("The provided JSON text has invalid syntax");
}

inline void JsonReader::readFromJson(JsonObject* root) {
	root->accept(*this, rapidjsonDocument);
}

enum class QueryType {
	IsInt,
	IsInt64,
	IsUint64,
	IsFloat,
	IsDouble,
	IsBool,
	IsString,
	IsObject,
	IsArray
};

class RapidjsonValueTypeValidator {
public:

#define RAPIDJSON_VALUE_VALIDATE(value, query, expectedType)												\
    if(!value. ## query())																					\
	    throw TypeMismatchException(std::string("Expected ") + expectedType + ", got " + getTypeFrom(value));

	static void validate(const rapidjson::Value& value, QueryType type) {
		switch (type) {
			case QueryType::IsInt:    RAPIDJSON_VALUE_VALIDATE(value, IsInt,    "Int");    break;
			case QueryType::IsInt64:  RAPIDJSON_VALUE_VALIDATE(value, IsInt64,  "Int64");  break;
			case QueryType::IsUint64: RAPIDJSON_VALUE_VALIDATE(value, IsUint64, "Uint64"); break;
			case QueryType::IsFloat:  RAPIDJSON_VALUE_VALIDATE(value, IsFloat,  "Float");  break;
			case QueryType::IsDouble: RAPIDJSON_VALUE_VALIDATE(value, IsDouble, "Double"); break;
			case QueryType::IsBool:   RAPIDJSON_VALUE_VALIDATE(value, IsBool,   "Bool");   break;
			case QueryType::IsString: RAPIDJSON_VALUE_VALIDATE(value, IsString, "String"); break;
			case QueryType::IsArray:  RAPIDJSON_VALUE_VALIDATE(value, IsArray,  "Array");  break;
			case QueryType::IsObject: RAPIDJSON_VALUE_VALIDATE(value, IsObject, "Object"); break;
		}
	}

#undef RAPIDJSON_VALUE_VALIDATE

private:
	static std::string getTypeFrom(const rapidjson::Value& value) {
		if (value.IsNumber()) {
			if (value.IsInt())    return "Int";
			if (value.IsInt64())  return "Int64";
			if (value.IsUint())   return "Uint";
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

inline void JsonReader::visit(JsonBasicValue* basicValue, rapidjson::Value& rapidjsonValue) {
	assert(!basicValue->IsPointToConst());

	if (rapidjsonValue.IsNull())
		return;

	switch (basicValue->storedType()) {
		case JsonBasicValue::StoredType::IntPtr: {
			RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsInt);

			auto value = std::any_cast<int*>(basicValue->storedValue());
			*value = rapidjsonValue.GetInt();
			break;
		} 

		case JsonBasicValue::StoredType::Int64Ptr: {
			RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsInt64);

			auto value = std::any_cast<int64_t*>(basicValue->storedValue());
			*value = rapidjsonValue.GetInt64();
			break;
		}

		case JsonBasicValue::StoredType::Uint64Ptr: {
			RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsUint64);

			auto value = std::any_cast<uint64_t*>(basicValue->storedValue());
			*value = rapidjsonValue.GetUint64();
			break;
		}

		case JsonBasicValue::StoredType::FloatPtr: {
			RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsFloat);

			auto value = std::any_cast<float*>(basicValue->storedValue());
			*value = rapidjsonValue.GetFloat();
			break;
		}

		case JsonBasicValue::StoredType::DoublePtr: {
			RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsDouble);

			auto value = std::any_cast<double*>(basicValue->storedValue());
			*value = rapidjsonValue.GetDouble();
			break;
		}

		case JsonBasicValue::StoredType::BoolPtr: {
			RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsBool);

			auto value = std::any_cast<bool*>(basicValue->storedValue());
			*value = rapidjsonValue.GetBool();
			break;
		}

		case JsonBasicValue::StoredType::StringPtr: {
			RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsString);

			auto value = std::any_cast<std::string*>(basicValue->storedValue());
			*value = rapidjsonValue.GetString();
			break;
		}
	}
}


inline void JsonReader::visit(JsonObject* object, rapidjson::Value& rapidjsonValue) {
	if (rapidjsonValue.IsNull())
		return;

	RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsObject);

	for (auto&& member : object->getMembers()) {
		auto name = member.name.c_str();
		throwIfFalse(rapidjsonValue.HasMember(name), MemberNotFoundException(name));

		try {
			member.value->accept(*this, rapidjsonValue[name]);
		}
		catch (std::logic_error& e) {
			throw MemberSerializationFailure(std::string("Deserialization of member \"") + 
				                             name + "\" failed: " + e.what());
		}

	}
}

inline std::size_t countNonNullElements(const rapidjson::Value& value) {
	assert(value.IsArray());

	std::size_t size = 0;
	for (auto&& elem : value.GetArray()) 
		if (!elem.IsNull())
			size++;

	return size;
}

inline void JsonReader::visit(JsonArray* array, rapidjson::Value& rapidjsonValue) {
	if (rapidjsonValue.IsNull())
		return;

	RapidjsonValueTypeValidator::validate(rapidjsonValue, QueryType::IsArray);

	auto jsonArraySize = countNonNullElements(rapidjsonValue);
	throwIfFalse(jsonArraySize <= array->size() || array->isResizable(), 
		         ArrayLengthMismatchException("Array size mismatch: JSON contains " + std::to_string(jsonArraySize) +
					                          " elements, but given array has fixed capacity of " + std::to_string(array->size()) +
					                          " elements and cannot be resized."));

	if (jsonArraySize > array->size())
		array->resize(jsonArraySize);



	auto elements = array->getElements();
	size_t elemIndex = 0;

	for (auto&& value : rapidjsonValue.GetArray()) 
		if (!value.IsNull())
			elements[elemIndex++]->accept(*this, value);
}


}  // namespace detail

inline MemberSerializationFailure::MemberSerializationFailure(std::string_view what):
	std::logic_error(what.data())
{
}

inline MemberNotFoundException::MemberNotFoundException(std::string_view member) :
	std::logic_error(std::string("JSON doesn't match the struct: required field \"").append(member).append("\" not found"))
{
}

inline TypeMismatchException::TypeMismatchException(std::string_view what) :
	std::logic_error(what.data())
{
}

inline ArrayLengthMismatchException::ArrayLengthMismatchException(std::string_view what) :
	std::logic_error(what.data())
{
}

inline InvalidJsonException::InvalidJsonException(std::string_view what) :
	std::logic_error(what.data())
{
}

inline EmptyJsonStringException::EmptyJsonStringException() :
	std::logic_error("The JSON string to be parsed is empty")
{
}

}  // namespace rapidjson_util 


#endif