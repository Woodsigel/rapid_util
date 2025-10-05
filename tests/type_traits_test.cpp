#include "gmock/gmock.h"
#include "rapid_util/rapid_util_preprocessor.h"
#include <cstdint>
#include <tuple>

using namespace rapidjson_util::detail;

TEST(JsonValueTypeTraitTest, SupportValidJsonTypes) {
	static_assert(is_json_serializable_type_v<int>);
	static_assert(is_json_serializable_type_v<int8_t>);
	static_assert(is_json_serializable_type_v<int32_t>);
	static_assert(is_json_serializable_type_v<int64_t>);
	static_assert(is_json_serializable_type_v<uint64_t>);
	static_assert(is_json_serializable_type_v<bool>);
	static_assert(is_json_serializable_type_v<std::string>);
	static_assert(is_json_serializable_type_v<float>);
	static_assert(is_json_serializable_type_v<double>);
}

TEST(JsonValueTypeTraitTest, RejectUnserializableTypes) {
	using aUnSerialableType = std::stringstream;

	static_assert(!is_json_serializable_type_v<int*>, "Pointers are not allowed");
	static_assert(!is_json_serializable_type_v<char*>, "Using std::string for parsing string");
	static_assert(!is_json_serializable_type_v<const std::string>, "Const-qualified types are not allowed");
	static_assert(!is_json_serializable_type_v<aUnSerialableType>, "Not a valid JSON ValueType");
}

TEST(JsonValueTypeTraitTest, ValidatesTupleSerializableElementTypes) {
	
	static_assert(is_json_serializable_tuple_v<std::tuple<int, double, float>>, 
		         "Tuples with primitive types are serializable");

	static_assert(is_json_serializable_tuple_v<std::tuple<int, double, std::tuple<std::string, std::vector<float>>>>, 
		          "Nested tuples are serializable if all elements are valid");

	using aUnSerialableType = std::stringstream;
	static_assert(!is_json_serializable_tuple_v < std::tuple<int, double, std::tuple<aUnSerialableType>>>,
		          "Tuples with any non-serializable elements are rejected");

}
