#include "gmock/gmock.h"
#include "rapid_util/rapid_util_preprocessor.h"
#include <cstdint>
#include <tuple>

using namespace rapidjson_util::detail;

TEST(JsonValueTypeTraitTest, SupportsValidJsonTypes) {
	static_assert(is_json_serializable_primitive_type_v<int>);
	static_assert(is_json_serializable_primitive_type_v<int8_t>);
	static_assert(is_json_serializable_primitive_type_v<int32_t>);
	static_assert(is_json_serializable_primitive_type_v<int64_t>);
	static_assert(is_json_serializable_primitive_type_v<uint64_t>);
	static_assert(is_json_serializable_primitive_type_v<bool>);
	static_assert(is_json_serializable_primitive_type_v<std::string>);
	static_assert(is_json_serializable_primitive_type_v<float>);
	static_assert(is_json_serializable_primitive_type_v<double>);
	static_assert(is_json_serializable_primitive_type_v<std::shared_ptr<int>>);
}

TEST(JsonValueTypeTraitTest, RejectsUnserializableTypes) {
	using aUnSerialableType = std::stringstream;

	static_assert(!is_json_serializable_primitive_type_v<int*>, "Raw pointers are not allowed, use std::shared_ptr instead");
	static_assert(!is_json_serializable_primitive_type_v<char*>, "Using std::string for parsing string");
	static_assert(!is_json_serializable_primitive_type_v<const std::string>, "Const-qualified types are not allowed");
	static_assert(!is_json_serializable_primitive_type_v<aUnSerialableType>, "Not a valid JSON ValueType");
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

struct AStruct {

};

template<typename T>
struct TypeHolder {

};

TEST(JsonValueTypeTraitTest, IdentifiesContainersWithSharedPtrElements) {
	static_assert(has_shared_ptr_elements<std::vector<std::shared_ptr<int>>>::value);
	static_assert(has_shared_ptr_elements<std::shared_ptr<std::vector<std::shared_ptr<std::string>>>>::value);

	static_assert(has_shared_ptr_elements<std::list<std::shared_ptr<double>>>::value);
	static_assert(has_shared_ptr_elements<std::shared_ptr<std::list<std::shared_ptr<float>>>>::value);

	static_assert(has_shared_ptr_elements<std::array<std::shared_ptr<float>, 5>>::value);
	static_assert(has_shared_ptr_elements<std::shared_ptr<std::array<std::shared_ptr<bool>, 10>>>::value);

	static_assert(!has_shared_ptr_elements<std::vector<std::shared_ptr<AStruct>>>::value);
	static_assert(!has_shared_ptr_elements<TypeHolder<std::shared_ptr<bool>>>::value);
}
