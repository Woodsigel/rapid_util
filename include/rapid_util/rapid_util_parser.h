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

#ifndef __RAPID_UTIL_PARSER_H__
#define __RAPID_UTIL_PARSER_H__

#include "rapid_util_preprocessor.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string_view>
#include <string>
#include <functional>
#include <cstdint>
#include <any>
#include <stdexcept>

namespace rapidjson_util {

/**
 * @brief Exception thrown when a JSON object member fails to deserialize
 */
class MemberSerializationFailure : public std::runtime_error {
public:
	MemberSerializationFailure(std::string_view what);
};


/**
 * @brief Exception thrown when required JSON member is missing
 */
class MemberNotFoundException : public MemberSerializationFailure {
public:
	MemberNotFoundException(std::string_view member);
};


/**
 * @brief Exception thrown when JSON value type doesn't match the expected type of structs
 */
class TypeMismatchException : public MemberSerializationFailure {
public:
	TypeMismatchException(std::string_view what);
};


/**
 * @brief Exception thrown when JSON array size doesn't match the array size of structs
 */
class ArrayLengthMismatchException : public MemberSerializationFailure {
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

class JsonPrimitiveValue;
class JsonObject;
class JsonNullableObject;
class JsonArray;
class JsonNullableArray;

template<typename Exception>
void ThrowExceptionUnless(bool condition, Exception&& exception) {
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
	virtual void visit(JsonPrimitiveValue*, rapidjson::Value& rapidjsonValue) = 0;
	virtual void visit(JsonObject*, rapidjson::Value& rapidjsonValue) = 0;
	virtual void visit(JsonNullableObject* object, rapidjson::Value& rapidjsonValue) = 0;
	virtual void visit(JsonArray*, rapidjson::Value& rapidjsonValue) = 0;
	virtual void visit(JsonNullableArray* array, rapidjson::Value& rapidjsonValue) = 0;

	virtual ~JsonVisitor() = default;
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
	std::string writeToJson(JsonObject* root);

	void visit(JsonPrimitiveValue* primitiveValue, rapidjson::Value& jsonOutput) override;
	void visit(JsonObject* object, rapidjson::Value& jsonOutput) override;
	void visit(JsonNullableObject* object, rapidjson::Value& jsonOutput) override;
	void visit(JsonArray* array, rapidjson::Value& jsonOutput) override;
	void visit(JsonNullableArray* array, rapidjson::Value& jsonOutput) override;

private:
	void writeObjectMembers(JsonObject* object, rapidjson::Value& jsonOutput);
	void writeArrayMembers(JsonArray* array, rapidjson::Value& jsonOutput);

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
	 
	void visit(JsonPrimitiveValue* primitiveValue, rapidjson::Value& jsonInput) override;
	void visit(JsonObject* object, rapidjson::Value& jsonInput) override;
	void visit(JsonNullableObject* object, rapidjson::Value& jsonInput) override;
	void visit(JsonArray* array, rapidjson::Value& jsonInput) override;
	void visit(JsonNullableArray* array, rapidjson::Value& jsonInput) override;

private:
	void readObjectMembers(JsonObject* object, rapidjson::Value& jsonInput);
	void readArrayElements(JsonArray* array, rapidjson::Value& jsonInput);

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
	virtual ~JsonValue() = default;
};


class JsonPrimitiveValue : public JsonValue {
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

	enum OwnershipType {
		Raw, 
		StdOptional
	};

	/**
      * Constructs a JsonPrimitiveValue that maintains a pointer to a struct member.
      * The member can be a regular attribute or a optional wrapper, only std::optional
	  * members are allowed to be none (lack of a value).
      *
      * @template param T The member type (e.g. int, std::optional<int>, const std::string, etc.)
      * @param _value Pointer to the struct member that will be updated
      */
	template<typename T>
	JsonPrimitiveValue(T* _value) : value(_value) {
		type = getStoredType<T>();
		ptrOwnershipType = getOwnershipType<T>();
		isNone = checkIsNone<T>(_value);
		pointToConst = std::is_const_v<T>;

		if (isNone)
			assert(OwnershipType::StdOptional == ptrOwnershipType);
	}

	/**
      * @return true if the pointed member is const-qualified
      */
	bool isPointToConst() const {
		return pointToConst;
	}

	/**
	  * @return The underlying data type of the pointed member
      */
	StoredType storedType() const {
		return type;
	}

	/**
      * @return How the member is owned (raw pointer or pointer to a optional wrapper)
      */
	OwnershipType ownershipType() const {
		return ptrOwnershipType;
	}


	/**
	  * Checks if the struct's member is currently none (lack of a value).
	  * @pre ownershipType() must be OwnershipType::StdOptional (pointee must be an std::optional)
	  */
	bool isPointeeNone() const {
		assert(OwnershipType::StdOptional == ptrOwnershipType);
		
		return isNone;
	}

	/**
	 * Resets referenced std::optional struct's member to std::nullopt.
	 *
	 * @pre ownershipType() must be OwnershipType::StdOptional (pointee must be an std::optional)
	 * @pre isPointToConst() must be false (struct's member must be non-const qualified)
	 *
	 */
	void resetOptionalPointee() {
		assert(ownershipType() == OwnershipType::StdOptional);
		assert(!isPointToConst());
		
		#define RESET_TO_NONE(storedType, CXXType)										\
		    case StoredType::storedType: {												\
				auto value = std::any_cast<std::optional<CXXType>*>(storedValue());		\
				value->reset();															\
				break;                                                                  \
			}

		switch (storedType()) {
			RESET_TO_NONE(IntPtr, int)
			RESET_TO_NONE(Int64Ptr, int64_t)
			RESET_TO_NONE(Uint64Ptr, uint64_t)
			RESET_TO_NONE(FloatPtr, float)
			RESET_TO_NONE(DoublePtr, double)
			RESET_TO_NONE(BoolPtr, bool)
			RESET_TO_NONE(StringPtr, std::string)
		}

        #undef RESET_TO_NONE

		isNone = true;
	}

	/**
	  * Reinitialize referenced std::optional struct's member from std::nullopt.
	  *
	  * @pre ownershipType() must be OwnershipType::StdOptional (pointee must be an std::optional)
	  * @pre isPointToConst() must be false (struct's member must be non-const qualified)
	  *
	  */
	void reinitOptionalPointeeIfNone() {
		assert(ownershipType() == OwnershipType::StdOptional);
		assert(!isPointToConst());

		if (!isPointeeNone())
			return;


        #define REINITIALIZE(storedType, CXXType)                                        \
		    case JsonPrimitiveValue::StoredType::storedType: {                           \
				auto value = std::any_cast<std::optional<CXXType>*>(storedValue());      \
				*value = CXXType{};                                                      \
				break;                                                                   \
			}

		switch (storedType()) {
			REINITIALIZE(IntPtr, int)
			REINITIALIZE(Int64Ptr, int64_t)
			REINITIALIZE(Uint64Ptr, uint64_t)
			REINITIALIZE(FloatPtr, float)
			REINITIALIZE(DoublePtr, double)
			REINITIALIZE(BoolPtr, bool)
			REINITIALIZE(StringPtr, std::string)
		}

        #undef REINITIALIZE

		isNone = false;
	}

	/**
	  * @brief Unwrap a const pointer to the underlying data for read-only access.
	  *
	  * @pre If ownership type is StdOptional, isPointeeNone() must be false
	  * @pre isPointToConst() must be true (member is const-qualified)
	  *
	  */
	template<typename T>
	const T* unwrapConstPointer() {
		if(ownershipType() == OwnershipType::StdOptional) assert(!isPointeeNone());
		assert(isPointToConst());
	
		if (OwnershipType::Raw == ownershipType())
			return std::any_cast<const T*>(storedValue());


		const std::optional<T>* opt = std::any_cast<const std::optional<T>*>(storedValue());
		return const_cast<const T*>(&(opt->value()));
	}

	/**
	 * @brief Unwraps a pointer to the underlying data for read-write access.
	 * 
	 * @pre If ownership type is StdOptional, isPointeeNone() must be false
	 * @pre isPointToConst() must be false (member is not const-qualified)
	 */ 
	template<typename T>
	T* unwrapPointer() {
		if (ownershipType() == OwnershipType::StdOptional) assert(!isPointeeNone());
		assert(!isPointToConst());

		if(OwnershipType::Raw == ownershipType())
			return std::any_cast<T*>(storedValue());


		std::optional<T>* opt = std::any_cast<std::optional<T>*>(storedValue());
		return &(opt->value());
	}


	/**
	  * Accepts a JSON visitor for serialization/deserialization operations.
	  * Modifications through the visitor will update the managed struct member.
	  *
	  * @param visitor The visitor processing JSON operations
	  * @param rapidjsonValue The RapidJSON value to read from or write to
	  */
	void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) override {
		visitor.visit(this, rapidjsonValue);
	}

private:
	bool pointToConst;
	OwnershipType ptrOwnershipType;
	StoredType type;
	std::any value;
	bool isNone;

	std::any storedValue() const {
		return value;
	}

	template<typename T>
	bool checkIsNone(const T* value) {
		if constexpr (is_std_optional_v<T>)
			return !value->has_value();  
		else
			return false;
	}

	template<typename T>
	constexpr OwnershipType getOwnershipType() {
		if constexpr (is_std_optional_v<T>) return OwnershipType::StdOptional;
		else return OwnershipType::Raw;
	}

	template<typename T>
	constexpr StoredType getStoredType() {
		using BaseType = remove_const_and_optional_t<T>;

		if constexpr (std::is_same_v<BaseType, int>)       return StoredType::IntPtr;
		else if constexpr (std::is_same_v<BaseType, int64_t>)  return StoredType::Int64Ptr;
		else if constexpr (std::is_same_v<BaseType, uint64_t>) return StoredType::Uint64Ptr;
		else if constexpr (std::is_same_v<BaseType, bool>)     return StoredType::BoolPtr;
		else if constexpr (std::is_same_v<BaseType, float>)    return StoredType::FloatPtr;
		else if constexpr (std::is_same_v<BaseType, double>)   return StoredType::DoublePtr;
		else if constexpr (std::is_same_v<BaseType, std::string>) return StoredType::StringPtr;
		else static_assert(false, "Unsupported type");
	}
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

	virtual ~JsonObject() = default;

protected:
	std::vector<JsonAttribute> members;
};


class JsonNullableObject : public JsonObject {
public:
	using ReferencedValueReinitializer = std::function <std::vector<JsonAttribute>()>;
	using ReferencedValueResetter = std::function<void()>;

	JsonNullableObject() : isNone(true) {}

	JsonNullableObject(const std::vector<JsonAttribute>& _members)
		: isNone(false) {
		members = _members;
	}


	bool isReinitializerAttached() const {
		return reinitializer != nullptr;
	}

	bool isResetterAttached() const {
		return resetter != nullptr;
	}

	bool isReferencedValueNone() const {
		return isNone;
	}


    /**
	  * @brief This function assigns the provided function objects to handle reinitialization
	  *        of referenced member values (e.g., with std::optional wrappers).
	  *
	  * @param _reinitializer Callback used to reinitialize a referenced value.
	  *                       E.g., For std::optional<myStruct>, after _reinitializer
	  *                       invocation, the std::optional<myStruct> object will be
	  *                       initialized from std::null_opt. Its attributes form a subtree
	  *                       that maintains its nested member pointers, which will be
	  *                       updated accordingly, and its attributes of the subtree will be
	  *                       returned from the _reinitializer callback handler.
	  */
	void attachReinitializer(ReferencedValueReinitializer _reinitializer) {
		assert(_reinitializer != nullptr);
		reinitializer = _reinitializer;
	}

	/**
	  * @brief This function assigns the provided function objects to handle resetting
	  *        of referenced member values (e.g., with std::optional wrappers).
	  *
	  * @param _resetter      Callback used to reset a referenced value.
	  *                       E.g., For std::optional<myStruct>, after _resetter
	  *                       invocation, the std::optional<myStruct> object will be
	  *                       set to std::null_opt, and the subtree of member pointers
	  *                       of this object will be removed accordingly.
	  */
	void attachResetter(ReferencedValueResetter _resetter) {
		assert(_resetter != nullptr);
		resetter = _resetter;
	}


	void resetReferencedValue() {
		assert(isResetterAttached());

		resetter();

		members.clear();
		isNone = true;
	}

	void reinitReferencedValue() {
		assert(isReinitializerAttached());

		members = reinitializer();
		isNone = false;
	}


	void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) override {
		visitor.visit(this, rapidjsonValue);
	}

private:
	ReferencedValueReinitializer reinitializer;
	ReferencedValueResetter resetter;
	bool isNone;
};


class JsonArray : public JsonValue {
public:
	using ArrayResizer = std::function<std::vector<std::shared_ptr<JsonValue>>(std::size_t)>;

	JsonArray(const std::vector<std::shared_ptr<JsonValue>>& _elements = {}, 
		      bool _allowNullableElems = false) :
		elements(_elements), 
		allowNullableElems(_allowNullableElems) {
	}

	void setArrayResizer(ArrayResizer _resizer) {
		assert(_resizer != nullptr);

		resizer = _resizer;
	}

	bool isResizable() const {
		return resizer != nullptr;
	}

	void resize(std::size_t newSize) {
		assert(resizer != nullptr);

		elements = resizer(newSize);
	}

	std::size_t size() const {
		return elements.size();
	}

	/**
      * @brief Checks whether the array's elements are permitted to lack a value
      *
      * @return true  - The array's elements may be none (lack of a value).
      * @return false - The array's elements always have a valid value.
      */
	bool allowNullElements() const {
		return allowNullableElems;
	}

	std::vector<std::shared_ptr<JsonValue>> getElements() const {
		return elements;
	}

	void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) override {
		visitor.visit(this, rapidjsonValue);
	}

	virtual ~JsonArray() = default;

protected:
	std::vector<std::shared_ptr<JsonValue>> elements;
	ArrayResizer resizer = nullptr;
	bool allowNullableElems;
};


class JsonNullableArray : public JsonArray {
public:
	using ReferencedValueResetter = std::function<void()>;
	using ReferencedValueReinitializer = std::function<std::vector<std::shared_ptr<JsonValue>>()>;

	JsonNullableArray(bool _allowNullableElems = false) : isNone(true) {
		allowNullableElems = _allowNullableElems;
	}

	JsonNullableArray(const std::vector<std::shared_ptr<JsonValue>>& _elements, 
		              bool _allowNullableElems = false) 
		: isNone(false) {
		elements = _elements;
		allowNullableElems = _allowNullableElems;
	}


	bool isReinitializerAttached() const {
		return reinitializer != nullptr;
	}

	bool isResetterAttached() const {
		return resetter != nullptr;
	}

	bool isReferencedValueNone() const {
		return isNone;
	}


	/**
	  * @brief This function assigns the provided function objects to handle reinitialization of
	  *        referenced array-like member values (e.g., with std::optional<std::tuple<...>> or
	  *        std::optional<std::vector<...>>).
	  *
	  * @param _reinitializer Callback used to reinitialize a member referenced value.
	  *                       E.g., For std::optional<std::tuple<...>>, after _reinitializer
	  *                       invocation, the std::optional<std::tuple<...>> object will be
	  *                       initialized from std::null_opt. Its contained elements represent
	  *                       a subtree that maintains nested member pointers. This subtree
	  *                       will be updated accordingly, and the elements of the updated
	  *                       subtree will be returned from the _reinitializer callback handler.
	  */
	void attachReinitializer(ReferencedValueReinitializer _reinitializer) {
		assert(_reinitializer != nullptr);
		reinitializer = _reinitializer;
	}

	/**
      * @brief This function assigns the provided function objects to handle resetting of referenced array-like
	  *        member values (e.g., with std::optional<std::tuple<...>> or std::optional<std::vector<...>>).
      *
      * @param _resetter      Callback used to reset a referenced value.
      *                       E.g., For std::optional<std::vector<int>>, after _resetter
      *                       invocation, the std::optional<std::vector<int>> object will be
      *                       set to std::null_opt, and the elements of this vector attribute
      *                       will be cleaned accordingly.
      */
	void attachResetter(ReferencedValueResetter _resetter) {
		assert(_resetter != nullptr);
		resetter = _resetter;
	}


	void resetReferencedValue() {
		assert(isResetterAttached());

		resetter();

		elements.clear();
		isNone = true;
	}

	void reinitReferencedValue() {
		assert(isReinitializerAttached());

		elements = reinitializer();
		isNone = false;
	}


	void accept(JsonVisitor& visitor, rapidjson::Value& rapidjsonValue) override {
		visitor.visit(this, rapidjsonValue);
	}

private:
	bool isNone;
	ReferencedValueResetter resetter = nullptr;
	ReferencedValueReinitializer reinitializer = nullptr;
};


inline std::string JsonWriter::writeToJson(JsonObject* root) {
	root->accept(*this, rapidjsonDocument);

	rapidjson::StringBuffer buffer;

	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	rapidjsonDocument.Accept(writer);

	return buffer.GetString();
}


inline void JsonWriter::visit(JsonPrimitiveValue* primitiveValue, rapidjson::Value& jsonOutput) {
	assert(primitiveValue->isPointToConst());

	if (primitiveValue->ownershipType() == JsonPrimitiveValue::StdOptional && 
		primitiveValue->isPointeeNone()) {
		jsonOutput.SetNull();
		return;
	}
		

	switch (primitiveValue->storedType()) {
		case JsonPrimitiveValue::StoredType::IntPtr: {
			auto value = primitiveValue->unwrapConstPointer<int>();
			jsonOutput.SetInt(*value);
			break;
		}

		case JsonPrimitiveValue::StoredType::Int64Ptr: {
			auto value = primitiveValue->unwrapConstPointer<int64_t>();
			jsonOutput.SetInt64(*value);
			break;
		}

		case JsonPrimitiveValue::StoredType::Uint64Ptr: {
			auto value = primitiveValue->unwrapConstPointer<uint64_t>();
			jsonOutput.SetUint64(*value);
			break;
		}

		case JsonPrimitiveValue::StoredType::BoolPtr: {
			auto value = primitiveValue->unwrapConstPointer<bool>();
			jsonOutput.SetBool(*value);
			break;
		}

		case JsonPrimitiveValue::StoredType::FloatPtr: {
			auto value = primitiveValue->unwrapConstPointer<float>();
			jsonOutput.SetFloat(*value);
			break;
		}

		case JsonPrimitiveValue::StoredType::DoublePtr: {
			auto value = primitiveValue->unwrapConstPointer<double>();
			jsonOutput.SetDouble(*value);
			break;
		}

		case JsonPrimitiveValue::StoredType::StringPtr: {
			auto value = primitiveValue->unwrapConstPointer<std::string>();
			jsonOutput.SetString(value->c_str(), static_cast<rapidjson::SizeType>(value->length()),
				                     rapidjsonDocument.GetAllocator());
			break;
		}
	}
}


inline void JsonWriter::visit(JsonObject* object, rapidjson::Value& jsonOutput) {
	writeObjectMembers(object, jsonOutput);
}


inline void JsonWriter::visit(JsonNullableObject* object, rapidjson::Value& jsonOutput) {
	if (object->isReferencedValueNone()) {
		jsonOutput.SetNull();
		return;
	}


	writeObjectMembers(object, jsonOutput);
}

inline void JsonWriter::writeObjectMembers(JsonObject* object, rapidjson::Value& jsonOutput){
	jsonOutput.SetObject();

	for (auto&& member : object->getMembers()) {
		rapidjson::Value name(member.name.c_str(), rapidjsonDocument.GetAllocator());

		rapidjson::Value value;
		member.value->accept(*this, value);

		jsonOutput.AddMember(name, value, rapidjsonDocument.GetAllocator());
	}
}


inline void JsonWriter::visit(JsonArray* array, rapidjson::Value& jsonOutput) {
	writeArrayMembers(array, jsonOutput);
}


inline void JsonWriter::visit(JsonNullableArray* array, rapidjson::Value& jsonOutput) {
	if (array->isReferencedValueNone()) {
		jsonOutput.SetNull();
		return;
	}


	writeArrayMembers(array, jsonOutput);
}


inline void JsonWriter::writeArrayMembers(JsonArray* array, rapidjson::Value& jsonOutput) {
	jsonOutput.SetArray();

	for (auto&& element : array->getElements()) {
		rapidjson::Value value;
		element->accept(*this, value);

		jsonOutput.PushBack(value, rapidjsonDocument.GetAllocator());
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
	static void validate(const rapidjson::Value& value, QueryType type) {
		#define RAPIDJSON_VALUE_VALIDATE(value, query, expectedType)												\
		    if(!value. ## query())																					\
			    throw TypeMismatchException(std::string("Expected ") + expectedType + ", got " + getTypeFrom(value));

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

        #undef RAPIDJSON_VALUE_VALIDATE
	}

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


inline void JsonReader::visit(JsonPrimitiveValue* primitiveValue, rapidjson::Value& jsonInput) {
	assert(!primitiveValue->isPointToConst());

	bool isStdOptional = primitiveValue->ownershipType() == JsonPrimitiveValue::StdOptional;
	if (isStdOptional && jsonInput.IsNull()) {
		primitiveValue->resetOptionalPointee();
		return;
	}


	if (isStdOptional) 
		primitiveValue->reinitOptionalPointeeIfNone();

	switch (primitiveValue->storedType()) {
		case JsonPrimitiveValue::StoredType::IntPtr: {
			RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsInt);

			auto value = primitiveValue->unwrapPointer<int>();
			*value = jsonInput.GetInt();
			break;
		} 

		case JsonPrimitiveValue::StoredType::Int64Ptr: {
			RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsInt64);

			auto value = primitiveValue->unwrapPointer<int64_t>();
			*value = jsonInput.GetInt64();
			break;
		}

		case JsonPrimitiveValue::StoredType::Uint64Ptr: {
			RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsUint64);

			auto value = primitiveValue->unwrapPointer<uint64_t>();
			*value = jsonInput.GetUint64();
			break;
		}

		case JsonPrimitiveValue::StoredType::FloatPtr: {
			RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsFloat);

			auto value = primitiveValue->unwrapPointer<float>();
			*value = jsonInput.GetFloat();
			break;
		}

		case JsonPrimitiveValue::StoredType::DoublePtr: {
			RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsDouble);

			auto value = primitiveValue->unwrapPointer<double>();
			*value = jsonInput.GetDouble();
			break;
		}

		case JsonPrimitiveValue::StoredType::BoolPtr: {
			RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsBool);

			auto value = primitiveValue->unwrapPointer<bool>();
			*value = jsonInput.GetBool();
			break;
		}

		case JsonPrimitiveValue::StoredType::StringPtr: {
			RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsString);

			auto value = primitiveValue->unwrapPointer<std::string>();
			*value = jsonInput.GetString();
			break;
		}
	}
}


inline void JsonReader::visit(JsonObject* object, rapidjson::Value& jsonInput) {
	readObjectMembers(object, jsonInput);
}


inline void JsonReader::visit(JsonNullableObject* object, rapidjson::Value& jsonInput) {
	if (jsonInput.IsNull())
		return object->resetReferencedValue();


	if(object->isReferencedValueNone())
		object->reinitReferencedValue();

	readObjectMembers(object, jsonInput);
}

inline void  JsonReader::readObjectMembers(JsonObject* object, rapidjson::Value& jsonInput) {
	RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsObject);

	for (auto&& member : object->getMembers()) {
		auto name = member.name.c_str();
		ThrowExceptionUnless(jsonInput.HasMember(name), 
			                 MemberNotFoundException(name));

		try {
			member.value->accept(*this, jsonInput[name]);
		}
		catch (MemberSerializationFailure& e) {
			throw MemberSerializationFailure(std::string("Deserialization of member \"") +
				name + "\" failed: " + e.what());
		}
	}
}


inline void JsonReader::visit(JsonArray* array, rapidjson::Value& jsonInput) {
	readArrayElements(array, jsonInput);
}


inline void JsonReader::visit(JsonNullableArray* array, rapidjson::Value& jsonInput) {
	if (jsonInput.IsNull())
		return array->resetReferencedValue();


	if (array->isReferencedValueNone())
		array->reinitReferencedValue();

	readArrayElements(array, jsonInput);
}

inline bool containNullElements(const rapidjson::Value& value) {
	assert(value.IsArray());

	for (auto&& elem : value.GetArray())
		if (elem.IsNull())
			return true;

	return false;
}

inline void JsonReader::readArrayElements(JsonArray* array, rapidjson::Value& jsonInput) {
	RapidjsonValueTypeValidator::validate(jsonInput, QueryType::IsArray);

	if (containNullElements(jsonInput))
		ThrowExceptionUnless(array->allowNullElements(),
			                 TypeMismatchException("JSON array contains null elements"));

	ThrowExceptionUnless(jsonInput.Size() == array->size() || array->isResizable(),
		                 ArrayLengthMismatchException(
						 "Array size mismatch: JSON contains " + std::to_string(jsonInput.Size()) +
						 " elements, but given array has fixed capacity of " + std::to_string(array->size()) +
						 " elements and cannot be resized."));



	if (jsonInput.Size() != array->size())
		array->resize(jsonInput.Size());

	auto elements = array->getElements();
	size_t elemIndex = 0;

	for (auto&& value : jsonInput.GetArray())
		elements[elemIndex++]->accept(*this, value);
}

}  // namespace detail

inline MemberSerializationFailure::MemberSerializationFailure(std::string_view what):
	std::runtime_error(what.data())
{
}

inline MemberNotFoundException::MemberNotFoundException(std::string_view member) :
	MemberSerializationFailure(std::string("JSON doesn't match the struct -- required field \"").append(member).append("\" not found"))
{
}

inline TypeMismatchException::TypeMismatchException(std::string_view what) :
	MemberSerializationFailure(what.data())
{
}

inline ArrayLengthMismatchException::ArrayLengthMismatchException(std::string_view what) :
	MemberSerializationFailure(what.data())
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