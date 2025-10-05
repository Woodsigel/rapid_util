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
    return detail::marshalImp(s);
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
    return detail::unmarshalImp(json, s);
}

namespace detail {

template<typename T>
std::shared_ptr<JsonBasicValue> createJsonBasicValueFrom(T& value) {
    static_assert(is_json_serializable_type_v<std::decay_t<T>>);

    return std::make_shared<JsonBasicValue>(&value);
}

template<typename T>
std::shared_ptr<JsonObject> createJsonObjectFrom(T& value) {
    static_assert(is_describable_struct_v<T>);

    auto object = std::make_shared<JsonObject>();
    buildJsonTree(value, *object);

    return object;
}

template<typename Sequence>
std::vector<std::shared_ptr<JsonValue>> convertToJsonValuesFrom(Sequence& sequence) {
    std::vector<std::shared_ptr<JsonValue>> values;

    for (auto&& it : sequence) {
        using elemType = std::decay_t<decltype(it)>;

        if constexpr (is_json_serializable_type_v<elemType>)
            values.push_back(createJsonBasicValueFrom(it));

        else if constexpr (is_describable_struct_v<elemType>)
            values.push_back(createJsonObjectFrom(it));
    }

    return values;
}

template<typename T>
std::shared_ptr<JsonArray> createJsonArrayFromSeq(T& sequence) {
    static_assert(is_json_serializable_sequential_container_v<std::decay_t<T>>);

    auto elements = convertToJsonValuesFrom(sequence);

    if constexpr (!std::is_const_v<std::remove_reference_t<T>> && is_json_serializable_dynamic_array_v<std::decay_t<T>>) {
        auto resizer = [sequencePtr = &sequence](std::size_t newSize) {
                        sequencePtr->resize(newSize);
                        return  convertToJsonValuesFrom(*sequencePtr); };

        return std::make_shared<JsonArray>(elements, resizer);
    }

    return  std::make_shared<JsonArray>(elements);
}

template<typename Tuple>
std::shared_ptr<JsonArray> createJsonArrayFromTup(Tuple& tuple) {
    static_assert(is_json_serializable_tuple_v<std::decay_t<Tuple>>);

    std::vector<std::shared_ptr<JsonValue>> elements;

    std::apply([&elements](auto&&... tupleArgs) {
        auto process = [&elements](auto&& arg) {
            using elemType = std::decay_t<decltype(arg)>;

            if constexpr (is_json_serializable_type_v<elemType>)
                elements.push_back(createJsonBasicValueFrom(arg));

            else if constexpr (is_describable_struct_v<elemType>)
                elements.push_back(createJsonObjectFrom(arg));

            else if constexpr (is_json_serializable_tuple_v<elemType>)
                elements.push_back(createJsonArrayFromTup(arg));

            else if constexpr (is_json_serializable_sequential_container_v<elemType>)
                elements.push_back(createJsonArrayFromSeq(arg));
        };

        (process(std::forward<decltype(tupleArgs)>(tupleArgs)), ...);
   }, tuple);

    return  std::make_shared<JsonArray>(elements);
}


template<typename Struct>
void buildJsonTree(Struct& s, JsonObject& object) {
    static_assert(is_describable_struct_v<Struct>, "Use RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro to describe the struct members beforehand.");

    std::vector<JsonAttribute> members;

    for_each(Descriptor<std::decay_t<Struct>>::member_descriptors, [&s, &members](auto desc) {
        auto name = std::string(desc.name());
        decltype(auto) valuePtr = &(s.*(desc.pointer()));

        using ValueType = std::decay_t<decltype(s.*(desc.pointer()))>;

        if constexpr (is_json_serializable_type_v<ValueType>)
            members.push_back(JsonAttribute{ name, createJsonBasicValueFrom(*valuePtr) });

        else if constexpr (is_json_serializable_sequential_container_v<ValueType>)
            members.push_back(JsonAttribute{ name, createJsonArrayFromSeq(*valuePtr) });

        else if constexpr (is_json_serializable_tuple_v<ValueType>)
            members.push_back(JsonAttribute{ name, createJsonArrayFromTup(*valuePtr) });

        else if constexpr (is_describable_struct_v<ValueType>) 
            members.push_back(JsonAttribute{ name, createJsonObjectFrom(*valuePtr) });
    });

    object.setMembers(members);
}


template<typename Struct>
std::string marshalImp(const Struct& s) {
    JsonObject root;
    buildJsonTree(s, root);

    JsonWriter writer;
    return writer.witeToJson(&root);
}

template<typename Struct>
void unmarshalImp(std::string_view json, Struct& s)  {
    JsonReader reader(json);

    JsonObject root;
    buildJsonTree(s, root);

    reader.readFromJson(&root);
}


#define RAPIDJSON_UTIL_CHECK_MEMBERS_ARE_SERIALIZABLE(C, members) \
        RAPIDJSON_UTIL_FOR_EACH(RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE, C, RAPIDJSON_UTIL_UNPACK members)

#define RAPIDJSON_UTIL_ASSERT_IS_SERIALIZABLE(C, member) \
        static_assert(rapidjson_util::detail::is_json_serializable_v<rapidjson_util::detail::member_type_t<decltype(&C::member)>>, "Member variable types must be compatible with JSON value types.");


#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMP(C, members)  template<> struct rapidjson_util::detail::Descriptor<C> {     \
     	static constexpr bool is_describable = true;                                                                   \
        static constexpr auto member_descriptors = make_typelist(                                                      \
                      RAPIDJSON_UTIL_REMOVE_FIRST_ARG(RAPIDJSON_UTIL_FOR_EACH(                                         \
                          RAPIDJSON_UTIL_MEMBER_META, C, RAPIDJSON_UTIL_UNPACK members)));                             \
        };


#define RAPIDJSON_UTIL_MEMBER_META(C, member)                                                      \
	, []{ struct rapidjsonUtilDesc {                                                               \
             static constexpr auto pointer() noexcept { return &C::member; }                       \
             static constexpr auto name() noexcept { return RAPIDJSON_UTIL_STRINGIFY(member); }    \
    }; return rapidjsonUtilDesc{}; }  () 

}  // namespace detail
}  // namespace rapidjson_util 

#define RAPIDJSON_UTIL_DESCRIBE_MEMBERS(C, members)                        \
        static_assert(std::is_class_v<C>);                                 \
        RAPIDJSON_UTIL_CHECK_MEMBERS_ARE_SERIALIZABLE(C, members)          \
        RAPIDJSON_UTIL_DESCRIBE_MEMBERS_IMP(C, members)


#endif