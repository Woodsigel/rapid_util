#include "gmock/gmock.h"
#include "rapid_util/rapid_util_preprocessor.h"
#include <cstdint>
#include <tuple>

using namespace rapidjson_util::detail;

TEST(JsonValueTypeTraitTest, SupportValidJsonTypes) {
	static_assert(is_json_serializable_primitive_type_v<int>);
	static_assert(is_json_serializable_primitive_type_v<int8_t>);
	static_assert(is_json_serializable_primitive_type_v<int32_t>);
	static_assert(is_json_serializable_primitive_type_v<int64_t>);
	static_assert(is_json_serializable_primitive_type_v<uint64_t>);
	static_assert(is_json_serializable_primitive_type_v<bool>);
	static_assert(is_json_serializable_primitive_type_v<std::string>);
	static_assert(is_json_serializable_primitive_type_v<float>);
	static_assert(is_json_serializable_primitive_type_v<double>);
	static_assert(is_json_serializable_primitive_type_v<std::optional<int>>);
}

TEST(JsonValueTypeTraitTest, RejectUnserializableTypes) {
	using aUnSerialableType = std::stringstream;

	static_assert(!is_json_serializable_primitive_type_v<int*>, "Raw pointers are not allowed, use std::optional instead");
	static_assert(!is_json_serializable_primitive_type_v<std::optional<int*>>);
	static_assert(!is_json_serializable_primitive_type_v<char*>, "Using std::string for parsing string");
	static_assert(!is_json_serializable_primitive_type_v<const std::string>, "Const-qualified types are not allowed");
	static_assert(!is_json_serializable_primitive_type_v<aUnSerialableType>, "Not a valid JSON ValueType");
}

TEST(JsonValueTypeTraitTest, ValidateTupleSerializableElementTypes) {
	
	static_assert(is_json_serializable_tuple_v<std::tuple<int, double, float>>, 
		         "Tuples with primitive types are serializable");

	static_assert(is_json_serializable_tuple_v<std::tuple<int, double, std::tuple<std::string, std::vector<float>>>>, 
		          "Nested tuples are serializable if all elements are valid");

	using aUnSerialableType = std::stringstream;
	static_assert(!is_json_serializable_tuple_v < std::tuple<int, double, std::tuple<aUnSerialableType>>>,
		          "Tuples with any non-serializable elements are rejected");

}

template<typename T>
struct TypeHolder {

};

TEST(JsonValueTypeTraitTest, IdentifyContainersWithNullableElementsUsingStdOptional) {
	using aUnSerialableType = std::stringstream;

	static_assert(has_std_optional_elements<std::vector<std::optional<int>>>::value);
	static_assert(has_std_optional_elements<std::optional<std::vector<std::optional<std::string>>>>::value);

	static_assert(has_std_optional_elements<std::list<std::optional<double>>>::value);
	static_assert(has_std_optional_elements<std::optional<std::list<std::optional<float>>>>::value);

	static_assert(has_std_optional_elements<std::array<std::optional<float>, 5>>::value);
	static_assert(has_std_optional_elements<std::optional<std::array<std::optional<bool>, 10>>>::value);

	static_assert(!has_std_optional_elements<std::vector<std::optional<aUnSerialableType>>>::value);
	static_assert(!has_std_optional_elements<TypeHolder<std::optional<bool>>>::value,
		          "TypeHolder is not a standard sequential container.");
}
