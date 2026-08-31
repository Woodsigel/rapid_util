#include <gmock/gmock.h>

#include "rapidjs_util/value.h"

using namespace rapidjson_util::detail;
using ::testing::Eq;
using ::testing::Ne;
using testing::ElementsAre;


TEST(ValueTest, ConstPtrIsImmutable) {
	std::shared_ptr<const float> f = std::make_shared<const float>(44.0f);
	std::shared_ptr<const float>* ptr = &f;
	Value v(ptr);

	ASSERT_THAT(v.isModifiable(), Eq(false));
}

TEST(ValueTest, NonConstPtrIsMutable) {
	std::shared_ptr<float> f = std::make_shared<float>(21.f);
	std::shared_ptr<float>* ptr = &f;
	Value v(ptr);

	ASSERT_THAT(v.isModifiable(), Eq(true));
}

TEST(ValueTest, RawPtrCannotBeNone) {
	double d = 49.12;
	double* ptr = &d;
	Value v(ptr);

	ASSERT_THAT(v.canBeNone(), Eq(false));
}

TEST(ValueTest, WrappedPtrCanBeNone) {
	std::optional<double> opt = 665.12;
	std::optional<double>* ptr = &opt;
	Value v(ptr);

	ASSERT_THAT(v.canBeNone(), Eq(true));
}

TEST(ValueTest, WrappedConstPtrCannotBeNone) {
	std::optional<const double> opt = 665.12;
	std::optional<const double>* ptr = &opt;
	Value v(ptr);

	ASSERT_THAT(v.canBeNone(), Eq(false));
}

TEST(ValueTest, RawPtrIsAlwaysNotNone) {
	bool b = false;
	bool* ptr = &b;
	Value v(ptr);

	ASSERT_THAT(v.isNone(), Eq(false));
}

TEST(ValueTest, IsNoneIfWrappedPtrInitializedWithoutInitializer) {
	std::optional<int> i = std::nullopt;
	std::optional<int>* ptr = &i;
	Value v(ptr);

	ASSERT_THAT(v.isNone(), Eq(true));
}

TEST(ValueTest, IsNotNoneIfWrappedPtrInitializedWithInitializer) {
	std::optional<int> i = 72;
	std::optional<int>* ptr = &i;
	Value v(ptr);

	ASSERT_THAT(v.isNone(), Eq(false));
}

TEST(ValueTest, IsNoneAfterWrappedPtrSetNone) {
	std::shared_ptr<bool> b = std::make_shared<bool>(true);
	std::shared_ptr<bool>* ptr = &b;
	Value v(ptr);

	ASSERT_THAT(v.isNone(), Eq(false));

	v.setNone();

	ASSERT_THAT(v.isNone(), Eq(true));
}

TEST(ValueTest, PointeeIsNullAfterWrappedPtrSetNone) {
	std::shared_ptr<bool> b = std::make_shared<bool>(true);
	std::shared_ptr<bool>* ptr = &b;
	Value v(ptr);

	ASSERT_TRUE(b);

	v.setNone();

	ASSERT_THAT(b, Eq(nullptr));
}


class PrimitiveTest : public ::testing::Test {
protected:
	enum class PrimitiveType : unsigned {
		Bool   = 1 << 0,
		Int    = 1 << 1,
		Uint   = 1 << 2,
		Int64  = 1 << 3,
		UInt64 = 1 << 4,
		Float  = 1 << 5,
		Double = 1 << 6,
		String = 1 << 7
	};

	template<bool Const, typename ValueT>
	void ASSERT_TYPE(Primitive<Const, ValueT>& p, PrimitiveType expected) {
		const auto mask = static_cast<unsigned>(expected);

		ASSERT_THAT(p.isBool(), Eq((mask & static_cast<unsigned>(PrimitiveType::Bool)) != 0));
		ASSERT_THAT(p.isInt(), Eq((mask & static_cast<unsigned>(PrimitiveType::Int)) != 0));
		ASSERT_THAT(p.isUint(), Eq((mask & static_cast<unsigned>(PrimitiveType::Uint)) != 0));
		ASSERT_THAT(p.isInt64(), Eq((mask & static_cast<unsigned>(PrimitiveType::Int64)) != 0));
		ASSERT_THAT(p.isUint64(), Eq((mask & static_cast<unsigned>(PrimitiveType::UInt64)) != 0));
		ASSERT_THAT(p.isFloat(), Eq((mask & static_cast<unsigned>(PrimitiveType::Float)) != 0));
		ASSERT_THAT(p.isDouble(), Eq((mask & static_cast<unsigned>(PrimitiveType::Double)) != 0));
		ASSERT_THAT(p.isString(), Eq((mask & static_cast<unsigned>(PrimitiveType::String)) != 0));
	}
};

TEST_F(PrimitiveTest, PointeeIsDefaultInitializedAfterReinit) {
	float f = 312.56f;
	float* ptr = &f;
	Value v(ptr);

	ASSERT_FLOAT_EQ(f, 312.56f);

	v.reinit();

	ASSERT_FLOAT_EQ(f, 0.0f);
}

TEST_F(PrimitiveTest, IsPrimitiveIfConstructedWithPrimitiveMemPtrType) {
	std::optional<int> i = 91;
	std::optional<int>* ptr = &i;
	Value v(ptr);

	ASSERT_THAT(v.isPrimitive(), Eq(true));
}

TEST_F(PrimitiveTest, ConstructedWithBoolReportsOnlyBool) {
	std::shared_ptr<bool> b = std::make_shared<bool>(false);
	std::shared_ptr<bool>* ptr = &b;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::Bool);
	ASSERT_THAT(p.getBool(), Eq(false));
}

TEST_F(PrimitiveTest, ConstructedWithIntReportsOnlyInt) {
	int i = 19;
	int* ptr = &i;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::Int);
	ASSERT_THAT(p.getInt(), Eq(19));
}

TEST_F(PrimitiveTest, ConstructedWithUintReportsOnlyUint) {
	unsigned u = 999999u;
	unsigned* ptr = &u;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::Uint);
	ASSERT_THAT(p.getUint(), Eq(999999u));
}

TEST_F(PrimitiveTest, ConstructedWithInt64ReportsOnlyInt64) {
	int64_t i = -3729847612459881234;
	int64_t* ptr = &i;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::Int64);
	ASSERT_THAT(p.getInt64(), Eq(-3729847612459881234));
}

TEST_F(PrimitiveTest, ConstructedWithUint64ReportsOnlyUint64) {
	uint64_t u = 12845936721884639457;
	uint64_t* ptr = &u;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::UInt64);
	ASSERT_THAT(p.getUint64(), Eq(12845936721884639457));
}

TEST_F(PrimitiveTest, ConstructedWithFloatReportsOnlyFloat) {
	float f = 88.32f;
	float* ptr = &f;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::Float);
	ASSERT_FLOAT_EQ(p.getFloat(), 88.32f);
}

TEST_F(PrimitiveTest, ConstructedWithDoubleReportsOnlyDouble) {
	double d = 44.75;
	double* ptr = &d;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::Double);
	ASSERT_DOUBLE_EQ(p.getDouble(), 44.75);
}

TEST_F(PrimitiveTest, ConstructedWithStringReportsOnlyString) {
	std::optional<std::string> s = "I am a string";
	auto ptr = &s;
	Value v(ptr);
	Primitive p = v.asPrimitive();

	ASSERT_TYPE(p, PrimitiveType::String);
	ASSERT_THAT(p.getString(), Eq("I am a string"));
}


class ObjectTest : public ::testing::Test {
public:
	struct Deep {
		double dVal;
		int iVal;
	};

	struct Middle {
		std::string strVal;
		float fVal;
		std::optional<Deep> deep;
	};

	struct Top {
		Middle middle;
	};

protected:
	void MEMBER_INT_NEXT(Value::MemberIterator& it, std::string_view name, int value) {
		ASSERT_THAT(it->name(), Eq(name));
		ASSERT_THAT(it->value().isInt(), Eq(true));
		ASSERT_THAT(it->value().getInt(), Eq(value));
		++it;
	}

	void MEMBER_FLOAT_NEXT(Value::MemberIterator& it, std::string_view name, float value) {
		ASSERT_THAT(it->name(), Eq(name));
		ASSERT_THAT(it->value().isFloat(), Eq(true));
		ASSERT_FLOAT_EQ(it->value().getFloat(), value);
		++it;
	}

	void MEMBER_DOUBLE_NEXT(Value::MemberIterator& it, std::string_view name, double value) {
		ASSERT_THAT(it->name(), Eq(name));
		ASSERT_THAT(it->value().isDouble(), Eq(true));
		ASSERT_DOUBLE_EQ(it->value().getDouble(), value);
		++it;
	}

	void MEMBER_STRING_NEXT(Value::MemberIterator& it, std::string_view name, std::string_view value) {
		ASSERT_THAT(it->name(), Eq(name));
		ASSERT_THAT(it->value().isString(), Eq(true));
		ASSERT_THAT(it->value().getString(), Eq(value));
		++it;
	}

	void CHECK_MEMBER_OBJECT(Value::MemberIterator& it, std::string_view name) {
		ASSERT_THAT(it->name(), Eq(name));
		ASSERT_THAT(it->value().isObject(), Eq(true));
	}


	Top top = Top{ Middle{"Hello", 10.2f, Deep{53.12, 12}} };
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(ObjectTest::Deep, (dVal, iVal));
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(ObjectTest::Middle, (strVal, fVal, deep));
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(ObjectTest::Top, (middle));

bool operator==(const ObjectTest::Deep& lhs, const ObjectTest::Deep& rhs) {
	return lhs.dVal == rhs.dVal && lhs.iVal == rhs.iVal;
}

bool operator==(const ObjectTest::Middle& lhs, const ObjectTest::Middle& rhs) {
	return lhs.strVal == rhs.strVal &&
		   lhs.fVal   == rhs.fVal   &&
		   lhs.deep   == rhs.deep;
}

bool operator==(const ObjectTest::Top& lhs, const ObjectTest::Top& rhs) {
	return lhs.middle == rhs.middle;
}

TEST_F(ObjectTest, IsObjectIfConstructedWithObjectMemPtrType) {
	auto ptr = &top;
	Value v(ptr);

	ASSERT_THAT(v.isObject(), Eq(true));
}

TEST_F(ObjectTest, ObjPointeeIsDefaultInitializedAfterReinit) {
	auto ptr = &top;
	Value v(ptr);
	Object obj = v.asObject();

	auto& origin = top;
	Top defaultInit {};
	ASSERT_FALSE(origin == defaultInit);

	obj.reinit();

	ASSERT_TRUE(origin == defaultInit);
}

TEST_F(ObjectTest, WalkThroughObjectHierarchy) {
	auto ptr = &top.middle;
	Value v(ptr);
	Object obj = v.asObject();
	

	auto it = obj.begin();
	MEMBER_STRING_NEXT(it, "strVal", "Hello");
	MEMBER_FLOAT_NEXT(it, "fVal", 10.2f);
	CHECK_MEMBER_OBJECT(it, "deep");          

	auto deepObj = it->value().asObject();
	auto innerIt = deepObj.begin();
	MEMBER_DOUBLE_NEXT(innerIt, "dVal", 53.12);
	MEMBER_INT_NEXT(innerIt, "iVal", 12);
	ASSERT_THAT(innerIt, Eq(deepObj.end()));
                            
	ASSERT_THAT(++it, Eq(obj.end()));
}

TEST_F(ObjectTest, ObjMembersAreResetToDefaultAfterReinit) {
	auto ptr = &top.middle;
	Value v(ptr);
	Object obj = v.asObject();

	obj.reinit();

	auto it = obj.begin();
	MEMBER_STRING_NEXT(it, "strVal", "");
	MEMBER_FLOAT_NEXT(it, "fVal", 0.0f);
	CHECK_MEMBER_OBJECT(it, "deep");

	ASSERT_THAT(it->value().canBeNone(), Eq(true));
	ASSERT_THAT(it->value().isNone(), Eq(true));

	ASSERT_THAT(++it, Eq(obj.end()));
}

TEST_F(ObjectTest, SetNoneClearsObjectMembers) {
	auto ptr = &top.middle.deep;
	Value v(ptr);
	Object obj = v.asObject();

	auto beg = obj.begin();
	auto end = obj.end();
	ASSERT_FALSE(beg == end);
	ASSERT_TRUE(v.canBeNone());

	v.setNone();

	beg = obj.begin();
	end = obj.end();
	ASSERT_TRUE(beg == end);
}


class ArrayTest : public ::testing::Test {
protected:
	void ARRAY_BOOL_NEXT(Value::ArrayIterator& it, bool val) {
		ASSERT_TRUE(it->isBool());
		ASSERT_THAT(it->getBool(), Eq(val));
		++it;
	}

	void ARRAY_INT_NEXT(Value::ArrayIterator& it, int val) {
		ASSERT_TRUE(it->isInt());
		ASSERT_THAT(it->getInt(), Eq(val));
		++it;
	}

	void ARRAY_FLOAT_NEXT(Value::ArrayIterator& it, float val) {
		ASSERT_TRUE(it->isFloat());
		ASSERT_FLOAT_EQ(it->getFloat(), val);
		++it;
	}

	void ARRAY_STRING_NEXT(Value::ArrayIterator& it, std::string_view val) {
		ASSERT_TRUE(it->isString());
		ASSERT_THAT(it->getString(), Eq(val));
		++it;
	}
};

class HomogeneousArrayTest : public ArrayTest {};


TEST_F(HomogeneousArrayTest, IsArrayIfConstructedWithArrayMemPtrType) {
	std::list<int> arr;
	auto ptr = &arr;
	Value v(ptr);

	ASSERT_THAT(v.isArray(), Eq(true));
}

TEST_F(HomogeneousArrayTest, IsNotResizableWhenConstructedWithFixedArray) {
	std::array<bool, 3> arr;
	auto ptr = &arr;
	Value v(ptr);

	ASSERT_THAT(v.isResizable(), Eq(false));
}

TEST_F(HomogeneousArrayTest, IsResizableWhenConstructedWithDynamicArray) {
	std::vector<float> arr;
	auto ptr = &arr;
	Value v(ptr);

	ASSERT_THAT(v.isResizable(), Eq(true));
}

TEST_F(HomogeneousArrayTest, PointedArrayShouldHaveNewSizeAfterResize) {
	std::list<float> arr;
	auto ptr = &arr;
	Value v(ptr);

	ASSERT_THAT(arr.size(), Ne(10));

	v.resize(10);

	ASSERT_THAT(arr.size(), Eq(10));
}

TEST_F(HomogeneousArrayTest, CorrectlyCountsElements) {
	std::vector<int> arr = { 1, 2, 3 };
	auto ptr = &arr;
	Value v(ptr);

	ASSERT_THAT(v.size(), Eq(3u));
}


TEST_F(HomogeneousArrayTest, CanNotHoldNoneElemWhenConstructedWithoutOptionalOrSharedWrapper) {
	std::array<float, 5> arr;
	auto ptr = &arr;
	Value v(ptr);

	ASSERT_THAT(v.canHoldNoneElem(), Eq(false));
}

TEST_F(HomogeneousArrayTest, CanHoldNoneElemWhenConstructedWithOptionalOrSharedWrapper) {
	std::array<std::shared_ptr<float>, 5> arr;
	auto ptr = &arr;
	Value v(ptr);

	ASSERT_THAT(v.canHoldNoneElem(), Eq(true));
}

TEST_F(HomogeneousArrayTest, WalkThroughArray) {
	std::list<int> arr = { 4, 5 };
	auto ptr = &arr;
	Value v(ptr);

	auto it = v.arrayBegin();

	ARRAY_INT_NEXT(it, 4);
	ARRAY_INT_NEXT(it, 5);
	ASSERT_THAT(it, Eq(v.arrayEnd()));
}

TEST_F(HomogeneousArrayTest, PointedArrayCanBeModifiedUsingIteratorsAfterResize) {
	std::vector<int> arr;;
	auto ptr = &arr;
	Value v(ptr);

	v.resize(5);

	auto beg = v.arrayBegin();
	auto end = v.arrayEnd();

	int i = 0;
	for (auto it = beg; it != end; ++it)
		it->setInt(i++);

	ASSERT_THAT(arr, ElementsAre(0, 1, 2, 3, 4));
}

class HeterogeneousArrayTest : public ArrayTest {
protected:
	void SetUp() override {
		heterArray = { 40, "world", 
					 std::make_optional(std::tuple<bool, float>{true, 3.14f}) };
	}

	std::tuple<int, std::string, std::optional<std::tuple<bool, float>>> heterArray;
};

TEST_F(HeterogeneousArrayTest, HeterogeneousArrayIsNotResizable) {
	auto ptr = &heterArray;
	Value v(ptr);

	ASSERT_THAT(v.isResizable(), Eq(false));
}

TEST_F(HeterogeneousArrayTest, CorrectlyCountsElements) {
	auto ptr = &heterArray;
	Value v(ptr);

	ASSERT_THAT(v.size(), Eq(3u));
}

TEST_F(HeterogeneousArrayTest, WalkThroughArray) {
	auto ptr = &heterArray;
	Value v(ptr);

	auto it = v.arrayBegin();
	ARRAY_INT_NEXT(it, 40);
	ARRAY_STRING_NEXT(it, "world");

	ASSERT_THAT(it->isArray(), Eq(true));
	ASSERT_THAT(it->canBeNone(), Eq(true));

	auto innerIt = it->arrayBegin();
	ARRAY_BOOL_NEXT(innerIt, true);
	ARRAY_FLOAT_NEXT(innerIt, 3.14f);
	ASSERT_THAT(innerIt, Eq(it->arrayEnd()));

	ASSERT_THAT(++it, Eq(v.arrayEnd()));
}