#include "gmock/gmock.h"
#include "rapid_util/rapid_util.h"

struct PrimitiveTypeBlob {
	int IntNumber;
	int64_t Int64Number;
	uint64_t Uint64Number;
	bool BoolValue;
	float FloatNumber;
	double DoubleNumber;
	std::string Str;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(PrimitiveTypeBlob, (IntNumber, Int64Number, Uint64Number, BoolValue, FloatNumber, DoubleNumber, Str))

TEST(RapidUnmarshalTest, UnserializePrimitiveTypes) {
	std::string json(R"( {
							"IntNumber"    : 32,
							"Int64Number"  : -9223372036854775808,
							"Uint64Number" : 18446744073709551615,
							"BoolValue"    : true,
							"FloatNumber"  : 3.1415926,
							"DoubleNumber" : 2.7182818,
							"Str"          : "World"
						   } )");
	PrimitiveTypeBlob blob;

	rapidjson_util::unmarshal(json, blob);

	ASSERT_EQ(blob.IntNumber, 32);
	ASSERT_EQ(blob.Int64Number, -9223372036854775808LL);
	ASSERT_EQ(blob.Uint64Number, 18446744073709551615ULL); 
	ASSERT_EQ(blob.BoolValue, true);
	ASSERT_FLOAT_EQ(blob.FloatNumber, 3.1415926f);
	ASSERT_DOUBLE_EQ(blob.DoubleNumber, 2.7182818);
	ASSERT_EQ(blob.Str, "World");
}

struct JobInfo {
	std::string title;
	double salary;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(JobInfo, (title, salary))

struct JobPosting {
	std::vector<JobInfo> jobs;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(JobPosting, (jobs))

TEST(RapidUnmarshalTest, UnserializeHomogeneousArray) {
	std::string json(R"({
							"jobs" : 
							    [{
							        "title": "Software Engineer",
							        "salary": 85000.0
							    },
							    {
							        "title": "Product Manager", 
							        "salary": 95000.0
							    },
							    {
							        "title": "Data Scientist",
							        "salary": 92000.0
							    }]
						   })");

	JobPosting jobPosting;
	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_EQ(jobPosting.jobs.size(), 3);

	ASSERT_EQ(jobPosting.jobs[0].title, "Software Engineer");
	ASSERT_DOUBLE_EQ(jobPosting.jobs[0].salary, 85000.0);

	ASSERT_EQ(jobPosting.jobs[1].title, "Product Manager");
	ASSERT_DOUBLE_EQ(jobPosting.jobs[1].salary, 95000.0);

	ASSERT_EQ(jobPosting.jobs[2].title, "Data Scientist");
	ASSERT_DOUBLE_EQ(jobPosting.jobs[2].salary, 92000.0);
}

struct EventInfo {
	std::string event;
	std::string page;
	float duration;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(EventInfo, (event, page, duration))

struct ApiResponse {
	std::tuple<EventInfo, uint64_t, std::string> response;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(ApiResponse, (response))

TEST(RapidUnmarshalTest, UnserializeHeterogeneousArray) {
	std::string json(R"({
						"response": [
						    {
						        "event": "page_view",
						        "page": "/home",
						        "duration": 45.2
						    },
						    17053000005,
						    "session_12345"]
						})");

	ApiResponse apiRes;
	rapidjson_util::unmarshal(json, apiRes);

	const auto& [eventInfo, timeStamp, sessionId] = apiRes.response;
	ASSERT_EQ(eventInfo.event, "page_view");
	ASSERT_EQ(eventInfo.page, "/home");
	ASSERT_EQ(timeStamp, 17053000005ULL);
	ASSERT_EQ(sessionId, "session_12345");
}

TEST(RapidUnmarshalTest, IgnoreNullValue) {
	std::string json(R"({ "jobs" : null })");

	JobPosting jobPosting;
	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_THAT(jobPosting.jobs, ::testing::IsEmpty());
}


TEST(RapidUnmarshalTest, SkipsNullElements) {
	std::string json(R"({
					    "jobs": [
					        {
					            "title": "Frontend Developer",
					            "salary": 4800.0
					        },

					        null,

					        {
					            "title": "Backend Developer",
					            "salary": 5000.0
					        }]
					})");

	JobPosting jobPosting;
	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_EQ(jobPosting.jobs.size(), 2);  

	ASSERT_EQ(jobPosting.jobs[0].title, "Frontend Developer");
	ASSERT_DOUBLE_EQ(jobPosting.jobs[0].salary, 4800.0);

	ASSERT_EQ(jobPosting.jobs[1].title, "Backend Developer");
	ASSERT_DOUBLE_EQ(jobPosting.jobs[1].salary, 5000.0);
}

TEST(RapidUnmarshalTest, ExistingValuesUnaffectedByNullJsonField) {
	std::string json = R"({
							"title": null,
							"salary" : 6000.0
						  })";
	JobInfo job{ "C++ Dev", 5000 };

	ASSERT_EQ(job.title, "C++ Dev");
	ASSERT_DOUBLE_EQ(job.salary, 5000.0);

	rapidjson_util::unmarshal(json, job);

	ASSERT_EQ(job.title, "C++ Dev");
	ASSERT_DOUBLE_EQ(job.salary, 6000.0);
}


struct SomeStruct {
	int someAttr;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SomeStruct, (someAttr))

TEST(RAPID_UNMARSHAL_TEST, ThrowsOnEmptyJsonString) {
	std::string emptyString("");
	SomeStruct s;
	ASSERT_THROW(rapidjson_util::unmarshal(emptyString, s), rapidjson_util::EmptyJsonStringException);
}

TEST(RAPID_UNMARSHAL_TEST, ThrowsOnInvalidJsonString) {
	std::string invalidJSON(R"( { name : "Zhao", } )");
	SomeStruct s;
	ASSERT_THROW(rapidjson_util::unmarshal(invalidJSON, s), rapidjson_util::InvalidJsonException);
}

struct Employee {
	std::string name;
	int age;
	JobInfo jobInfo;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Employee, (age, name, jobInfo))

TEST(RAPID_UNMARSHAL_TEST, ThrowsWhenRequiredMemberMissing){
	std::string json(R"( { "name" : "Wu" } )");
	Employee p;

	try {
		rapidjson_util::unmarshal(json, p);
		FAIL() << "Expected MemberNotFoundException";
	}
	catch (rapidjson_util::MemberNotFoundException& e) {
		EXPECT_STREQ(e.what(), "JSON doesn't match the struct: required field \"age\" not found");
	}
}

TEST(RAPID_UNMARSHAL_TEST, ThrowsMemberDeserializationException) {
	std::string json(R"( { "name" : "Li", "age" : "42" } )");
	Employee p;

	try {
		rapidjson_util::unmarshal(json, p);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		EXPECT_STREQ(e.what(), "Deserialization of member \"age\" failed: Expected Int, got String");
	}
}

struct SomeFixedArray {
	std::array<bool, 3> arr;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SomeFixedArray, (arr))

TEST(RAPID_UNMARSHAL_TEST, ThrowWhenJsonArrayLargerThanStdFixedArray) {
	std::string json(R"( { "arr" : [false, true, true, false] } )");
	SomeFixedArray fixedArray;
	try {
		rapidjson_util::unmarshal(json, fixedArray);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		EXPECT_STREQ(e.what(), "Deserialization of member \"arr\" failed: Array size mismatch: JSON contains 4 elements,"
			                   " but given array has fixed capacity of 3 elements and cannot be resized.");
	}
}

struct SomeHeterogeneousArray {
	std::tuple<bool, Employee> heteroArray;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SomeHeterogeneousArray, (heteroArray))

TEST(RAPID_UNMARSHAL_TEST, ThrowWhenJsonArrayLargerThanStdTuple) {
	std::string json(R"( { 
							"heteroArray" : [false, {"name" : "Li", "age" : 24}, 1.82] 
                         } )");
	SomeHeterogeneousArray heteroArray;
	try {
		rapidjson_util::unmarshal(json, heteroArray);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		EXPECT_STREQ(e.what(), "Deserialization of member \"heteroArray\" failed: Array size mismatch: JSON contains 3 elements,"
			                   " but given array has fixed capacity of 2 elements and cannot be resized.");
	}
}

