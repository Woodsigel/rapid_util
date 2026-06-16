#include "gmock/gmock.h"
#include "rapid_util/rapid_util_preprocessor.h"

using namespace rapidjson_util::detail;

TEST(JsonValueTypeTraitTest, SupportValidJsonTypes) {
	static_assert(is_json_serializable_v<int>);
	static_assert(is_json_serializable_v<int32_t>);
	static_assert(is_json_serializable_v<int64_t>);
	static_assert(is_json_serializable_v<uint64_t>);
	static_assert(is_json_serializable_v<bool>);
	static_assert(is_json_serializable_v<std::string>);
	static_assert(is_json_serializable_v<float>);
	static_assert(is_json_serializable_v<double>);
	static_assert(is_json_serializable_v<std::optional<int>>);
}

TEST(JsonValueTypeTraitTest, RejectUnserializableTypes) {
	using aUnSerialableType = std::stringstream;

	static_assert(!is_json_serializable_v<int*>, "Raw pointers are not allowed, use std::optional instead");
	static_assert(!is_json_serializable_v<int&>, "References are not allowed, use std::optional instead");
	static_assert(!is_json_serializable_v<std::optional<int*>>);
	static_assert(!is_json_serializable_v<std::optional<int&>>);
	static_assert(!is_json_serializable_v<char*>, "Using std::string for parsing string");
	static_assert(!is_json_serializable_v<const std::string>, "Const-qualified types are not allowed");
	static_assert(!is_json_serializable_v<aUnSerialableType>, "Not a valid JSON ValueType");
}

TEST(JsonValueTypeTraitTest, ValidateTupleSerializableElementTypes) {
	
	// Tuples with primitive types are serializable
	static_assert(is_jsonable_tuple_v<std::tuple<int, double, float>>);

	// Nested tuples are serializable if all elements are valid
	static_assert(is_jsonable_tuple_v<std::tuple<int, double, std::tuple<std::string, std::vector<float>>>>);

	// Nested tuples wrapped by std::optional and std::shared_ptr are serializable 
	static_assert(is_jsonable_tuple_v<std::tuple<int,  std::optional<std::tuple<int, std::list<float>>>>>);

	using aUnSerialableType = std::stringstream;
	static_assert(!is_jsonable_tuple_v < std::tuple<int, double, std::tuple<aUnSerialableType>>>,
		          "Tuples with any non-serializable elements are rejected");

}

template<typename T>
struct TypeHolder {

};


TEST(JsonValueTypeTraitTest, IdentifyContainersWithNullableElementsUsingStdOptionalOrShared) {
	using aUnSerialableType = std::stringstream;

	static_assert(can_hold_null_elem<std::vector<std::optional<int>>>::value);
	static_assert(can_hold_null_elem<std::optional<std::vector<std::optional<std::string>>>>::value);


	static_assert(can_hold_null_elem<std::array<std::shared_ptr<bool>, 10>>::value);
	static_assert(can_hold_null_elem<std::shared_ptr<std::list<std::shared_ptr<std::string>>>>::value);

	static_assert(!can_hold_null_elem<std::vector<std::optional<aUnSerialableType>>>::value);
	static_assert(!can_hold_null_elem<TypeHolder<std::shared_ptr<bool>>>::value,
		          "TypeHolder is not a standard sequential container.");
}

TEST(WrapperTraitTest, DetectStdOptionalTypes) {
	static_assert(!is_std_optional_v<int>);
	static_assert(is_std_optional_v<std::optional<int>>);
	static_assert(is_std_optional_v<const std::optional<int>>);
	static_assert(is_std_optional_v<std::optional<int>&>);

	static_assert(std::is_same_v<remove_std_optional_t<double>, double>);
	static_assert(std::is_same_v<remove_std_optional_t<std::optional<double>>, double>);
	static_assert(std::is_same_v<remove_std_optional_t<std::optional<double&>>, double&>);
	static_assert(std::is_same_v<remove_std_optional_t<std::optional<double*>>, double*>);

	static_assert(std::is_same_v<remove_std_optional_t<const std::optional<float>>, const float>);
	static_assert(std::is_same_v<remove_std_optional_t<const std::optional<const float>>, const float>);
}

TEST(WrapperTraitTest, DetectStdSharedTypes) {
	static_assert(!is_std_shared_ptr_v<bool>);
	static_assert(is_std_shared_ptr_v<std::shared_ptr<bool>>);
	static_assert(is_std_shared_ptr_v<const std::shared_ptr<bool>>);
	static_assert(is_std_shared_ptr_v<std::shared_ptr<bool>&>);

	static_assert(std::is_same_v<remove_std_shared_ptr_t<float>, float>);
	static_assert(std::is_same_v<remove_std_shared_ptr_t<std::shared_ptr<float>>, float>);
	static_assert(std::is_same_v<remove_std_shared_ptr_t<std::shared_ptr<float&>>, float&>);
	static_assert(std::is_same_v<remove_std_shared_ptr_t<std::shared_ptr<float*>>, float*>);

	static_assert(std::is_same_v<remove_std_shared_ptr_t<const std::shared_ptr<int>>, const int>);
	static_assert(std::is_same_v<remove_std_shared_ptr_t<const std::shared_ptr<const int>>, const int>);
}

TEST(WrapperTraitTest, RmoveSharedAndOptionalWrapper) {
	static_assert(std::is_same_v<remove_shared_optional_t<int>, int>);
	static_assert(std::is_same_v<remove_shared_optional_t<std::shared_ptr<int>>,  int>);
	static_assert(std::is_same_v<remove_shared_optional_t<const std::shared_ptr<int>>,const int>);

	static_assert(std::is_same_v<remove_shared_optional_t<std::optional<float>>, float>);
	static_assert(std::is_same_v<remove_shared_optional_t<const std::optional<float>>, const float>);

	static_assert(std::is_same_v<remove_shared_optional_t<std::shared_ptr<std::optional<double>>>, std::optional<double>>);
	static_assert(std::is_same_v<remove_shared_optional_t<std::optional<std::shared_ptr<double>>>, std::shared_ptr<double>>);
}