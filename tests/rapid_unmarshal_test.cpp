#include "gmock/gmock.h"
#include "rapid_util/rapid_util.h"

struct PrimitiveFields {
	int IntNumber;
	int64_t Int64Number;
	uint64_t Uint64Number;
	bool BoolValue;
	float FloatNumber;
	double DoubleNumber;
	std::string Str;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(PrimitiveFields, (IntNumber, Int64Number, Uint64Number, BoolValue, FloatNumber, DoubleNumber, Str))

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
	PrimitiveFields blob;

	rapidjson_util::unmarshal(json, blob);

	ASSERT_EQ(blob.IntNumber, 32);
	ASSERT_EQ(blob.Int64Number, -9223372036854775808LL);
	ASSERT_EQ(blob.Uint64Number, 18446744073709551615ULL); 
	ASSERT_EQ(blob.BoolValue, true);
	ASSERT_FLOAT_EQ(blob.FloatNumber, 3.1415926f);
	ASSERT_DOUBLE_EQ(blob.DoubleNumber, 2.7182818);
	ASSERT_EQ(blob.Str, "World");
}


struct SomeIntStruct {
	int IntNumber;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SomeIntStruct, (IntNumber))

TEST(RAPID_UNMARSHAL_TEST, ThrowsForPrimitiveTypesWithoutOptionalWhenNull) {
	std::string json(R"( { "IntNumber" : null } )");
	SomeIntStruct s;

	try {
		rapidjson_util::unmarshal(json, s);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		EXPECT_STREQ(e.what(), "Deserialization of member \"IntNumber\" failed: Expected Int, got Null");
	}
}


struct OptionalPrimitiveFields {
	std::optional<int> IntNumber;
	std::optional<int64_t> Int64Number;
	std::optional<uint64_t> Uint64Number;
	std::optional<bool> Bool;
	std::optional<float> FloatNumber;
	std::optional<double> DoubleNumber;
	std::optional<std::string> Str;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(OptionalPrimitiveFields, (IntNumber, Int64Number, Uint64Number, Bool, FloatNumber, DoubleNumber, Str))

TEST(RapidUnmarshalTest, UnserializeNullablePrimitiveTypesWithOptionalWhenNull) {
	OptionalPrimitiveFields f;
	f.IntNumber = 53;
	f.Int64Number = 9132101254LL;
	f.Uint64Number = 1243744404370511615ULL;
	f.Bool = true;
	f.FloatNumber = 22.485f;
	f.DoubleNumber = 00.231;
	f.Str = "Str";

	std::string json(R"( {
							"IntNumber"    : null,
							"Int64Number"  : null,
							"Uint64Number" : null,
							"Bool"   : null,
							"FloatNumber"  : null,
							"DoubleNumber" : null,
							"Str"    : null
						   } )");

	rapidjson_util::unmarshal(json, f);

	ASSERT_EQ(f.IntNumber, std::nullopt);
	ASSERT_EQ(f.Int64Number, std::nullopt);
	ASSERT_EQ(f.Uint64Number, std::nullopt);
	ASSERT_EQ(f.Bool, std::nullopt);
	ASSERT_EQ(f.FloatNumber, std::nullopt);
	ASSERT_EQ(f.DoubleNumber, std::nullopt);
	ASSERT_EQ(f.Str, std::nullopt);
}

TEST(RapidUnmarshalTest, UnserializeNullablePrimitiveTypesWithOptionalWhenPopulated) {
	std::string json(R"( {
							"IntNumber"    : 315,
							"Int64Number"  : 5132101254,
							"Uint64Number" : 6143744404370511615,
							"Bool"   : true,
							"FloatNumber"  : 78.4859,
							"DoubleNumber" : 31.231,
							"Str"    : "World"
						   } )");

	OptionalPrimitiveFields f;
	rapidjson_util::unmarshal(json, f);


	ASSERT_EQ(f.IntNumber.value(), 315);
	ASSERT_EQ(f.Int64Number.value(), 5132101254LL);
	ASSERT_EQ(f.Uint64Number.value(), 6143744404370511615ULL);
	ASSERT_EQ(f.Bool.value(), true);
	ASSERT_FLOAT_EQ(f.FloatNumber.value(), 78.4859);
	ASSERT_DOUBLE_EQ(f.DoubleNumber.value(), 31.231);
	ASSERT_EQ(f.Str.value(), "World");
}


struct Credential {
	std::string username;
	std::string passwd;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Credential, (username, passwd))

struct Application {
	std::string version;
	Credential credential;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Application, (version, credential))

TEST(RapidUnmarshalTest, UnserializeNestedStruct) {
	Application app;

	auto json = R"({
				     "version": "2.1.0",
				     "credential": {
				         "username": "admin",
				         "passwd": "secret123"
						}
				   })";

	rapidjson_util::unmarshal(json, app);

	ASSERT_EQ(app.version, "2.1.0");
	ASSERT_EQ(app.credential.username, "admin");
	ASSERT_EQ(app.credential.passwd, "secret123");
}

TEST(RapidUnmarshalTest, ThrowForNestedStructWithoutOptionalWhenRequiredObjectMemberIsNull) {
	auto json = R"({
				     "version": "1.1.2",
				     "credential": null
				   })";

	Application app;

	try {
		rapidjson_util::unmarshal(json, app);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		EXPECT_STREQ(e.what(), "Deserialization of member \"credential\" failed: Expected Object, got Null");
	}
}

struct DatabaseConfig {
	std::string host;
	int port;
	std::optional<Credential> credential;  
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(DatabaseConfig, (host, port, credential))

TEST(RapidUnmarshalTest, UnerializeNestedStructWithOptionalWhenNull) {
	auto json = R"( {
					"host": "localhost",
					"port": 4212,
					"credential": null
					})";

	DatabaseConfig config;
	rapidjson_util::unmarshal(json, config);

	ASSERT_EQ(config.host, "localhost");
	ASSERT_EQ(config.port, 4212);
	ASSERT_EQ(config.credential, std::nullopt);
}

TEST(RapidUnmarshalTest, UnerializeNestedStructWithOptionalWhenPopulated) {
	auto json = R"( {
					"host": "127.0.0.1",
					"port": 65432,
					"credential": {
								  "username": "admin",
								  "passwd": "secret123"
								}
					})";

	DatabaseConfig config;
	rapidjson_util::unmarshal(json, config);

	ASSERT_EQ(config.host, "127.0.0.1");
	ASSERT_EQ(config.port, 65432);
	ASSERT_NE(config.credential, std::nullopt);
	ASSERT_EQ(config.credential->username, "admin");
	ASSERT_EQ(config.credential->passwd, "secret123");
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

TEST(RapidUnmarshalTest, UnserializeHomogeneousArrayWhenEmpty) {
	JobPosting jobPosting;
	jobPosting.jobs.emplace_back(JobInfo{ "Accountant", 90000 });
	jobPosting.jobs.emplace_back(JobInfo{ "HR", 50000 });

	ASSERT_TRUE(!jobPosting.jobs.empty());

	std::string json(R"({ "jobs" : [] })");
	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_TRUE(jobPosting.jobs.empty());
}

struct JobPostingWithOptionalDetails {
	std::optional<std::vector<JobInfo>> jobs;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(JobPostingWithOptionalDetails, (jobs))

TEST(RapidUnmarshalTest, UnserializeNullableHomogeneousArrayWithOptionalWhenNull) {
	JobPostingWithOptionalDetails jobPosting;
	jobPosting.jobs = std::vector<JobInfo>{};
	jobPosting.jobs->emplace_back(JobInfo{"Business Manager", 20000});

	std::string json(R"({ "jobs" : null })");

	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_EQ(jobPosting.jobs, std::nullopt);
}

TEST(RapidUnmarshalTest, ThrowForHomogeneousArrayWithoutOptionalWhenRequiredArrayIsNull) {
	std::string json(R"({ "jobs" : null })");

	try {
		JobPosting jobPosting;
		rapidjson_util::unmarshal(json, jobPosting);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		EXPECT_STREQ(e.what(), "Deserialization of member \"jobs\" failed: Expected Array, got Null");
	}
}

TEST(RapidUnmarshalTest, UnserializeNullableHomogeneousArrayWithOptionalWhenEmpty) {
	JobPostingWithOptionalDetails jobPosting;
	jobPosting.jobs = std::nullopt;

	std::string json(R"({ "jobs" : [] })");

	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_NE(jobPosting.jobs, std::nullopt);
	ASSERT_TRUE(jobPosting.jobs->empty());
}

TEST(RapidUnmarshalTest, UnserializeNullableHomogeneousArrayWithOptionalWhenPopulated) {
	std::string json(R"({
        "jobs": [
            {
                "title": "QA Engineer",
                "salary": 72000.0
            },
            {
                "title": "Systems Architect", 
                "salary": 125000.0
            },
            {
                "title": "Mobile Developer",
                "salary": 95000.0
            }
        ]
    })");

	JobPostingWithOptionalDetails jobPosting;
	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_EQ(jobPosting.jobs->size(), 3);

	ASSERT_EQ((*jobPosting.jobs)[0].title, "QA Engineer");
	ASSERT_DOUBLE_EQ((*jobPosting.jobs)[0].salary, 72000.0);

	ASSERT_EQ((*jobPosting.jobs)[1].title, "Systems Architect");
	ASSERT_DOUBLE_EQ((*jobPosting.jobs)[1].salary, 125000.0);

	ASSERT_EQ((*jobPosting.jobs)[2].title, "Mobile Developer");
	ASSERT_DOUBLE_EQ((*jobPosting.jobs)[2].salary, 95000.0);
}


TEST(RapidUnmarshalTest, ThrowForHomogeneousArrayWithoutOptionalElemsWhenRequiredArrayContainsNullElements) {
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

	try {
		JobPosting jobPosting;
		rapidjson_util::unmarshal(json, jobPosting);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		ASSERT_STREQ(e.what(), "Deserialization of member \"jobs\" failed: JSON array contains null elements");
	}
}

struct JobPostingWithOptionalJobInfo {
	std::vector<std::optional<JobInfo>> jobs;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(JobPostingWithOptionalJobInfo, (jobs))

TEST(RapidUnmarshalTest, UnserializeHomogeneousArrayHavinghSharedPtrElemsWhenContainNulls) {
	std::string json(R"({
        "jobs": [
					 {
					     "title": "Senior DevOps Engineer", 
					     "salary": 135000.0
					 },
					 null,
					 null,
					 {
					     "title": "Security Analyst",
					     "salary": 110000.0
					 }
               ]
			})");

	JobPostingWithOptionalJobInfo jobPosting;
	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_EQ(jobPosting.jobs.size(), 4);

	ASSERT_EQ(jobPosting.jobs[0]->title, "Senior DevOps Engineer");
	ASSERT_DOUBLE_EQ(jobPosting.jobs[0]->salary, 135000.0);

	ASSERT_EQ(jobPosting.jobs[1], std::nullopt);

	ASSERT_EQ(jobPosting.jobs[2], std::nullopt);

	ASSERT_EQ(jobPosting.jobs[3]->title, "Security Analyst");
	ASSERT_DOUBLE_EQ(jobPosting.jobs[3]->salary, 110000.0);
}

struct OptionalJobPostingWithOptionalJobInfo {
	std::optional<std::vector<std::optional<JobInfo>>> jobs;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(OptionalJobPostingWithOptionalJobInfo, (jobs))

TEST(RapidUnmarshalTest, UnserializeHomogeneousArrayWithOptionalHavingSharedPtrElemsWhenContainNulls) {
	std::string json(R"({
        "jobs": [
            {
                "title": "Senior C++ Engineer", 
                "salary": 145000.0
            },
	        null,
            {
                "title": "Business Analyst",
                "salary": 310000.0
            }
        ]
    })");

	OptionalJobPostingWithOptionalJobInfo jobPosting;
	rapidjson_util::unmarshal(json, jobPosting);

	ASSERT_EQ(jobPosting.jobs->size(), 3);

	ASSERT_EQ((*jobPosting.jobs)[0]->title, "Senior C++ Engineer");
	ASSERT_DOUBLE_EQ((*jobPosting.jobs)[0]->salary, 145000.0);

	ASSERT_EQ((*jobPosting.jobs)[1], std::nullopt);

	ASSERT_EQ((*jobPosting.jobs)[2]->title, "Business Analyst");
	ASSERT_DOUBLE_EQ((*jobPosting.jobs)[2]->salary, 310000.0);
}

struct EventInfo {
	std::string event;
	std::string page;
	std::optional<float> duration;
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
						        "duration": 42.35
						    },
						    17053000005,
						    "session_12345"]
						})");

	ApiResponse apiRes;
	rapidjson_util::unmarshal(json, apiRes);

	const auto& [eventInfo, timeStamp, sessionId] = apiRes.response;
	ASSERT_EQ(eventInfo.event, "page_view");
	ASSERT_EQ(eventInfo.page, "/home");
	ASSERT_FLOAT_EQ(*eventInfo.duration, 42.35);
	ASSERT_EQ(timeStamp, 17053000005ULL);
	ASSERT_EQ(sessionId, "session_12345");
}

TEST(RapidUnmarshalTest, ThrowForHeterogeneousArrayWithoutSharedPtrWhenRequiredTupleIsNull) {
	std::string json(R"({ "response": null })");

	try {
		ApiResponse apiRes;
		rapidjson_util::unmarshal(json, apiRes);
		FAIL() << "Expected MemberSerializationFailure";
	}
	catch (rapidjson_util::MemberSerializationFailure& e) {
		EXPECT_STREQ(e.what(), "Deserialization of member \"response\" failed: Expected Array, got Null");
	}
}

struct OptionalApiResponse {
	std::optional<std::tuple<EventInfo, uint64_t, std::string>> response;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(OptionalApiResponse, (response))

TEST(RapidUnmarshalTest, UnserializeHeterogeneousArrayWithOptionalWhenNull) {
	std::string json(R"({
						"response": null
						})");

	OptionalApiResponse apiRes;
	apiRes.response = std::make_tuple(EventInfo{"page_view", "/home", 23.50f}, 
		                              37053240001, "arbitrary_session");

	ASSERT_NE(apiRes.response, std::nullopt);

	rapidjson_util::unmarshal(json, apiRes);

	ASSERT_EQ(apiRes.response, std::nullopt);
}

TEST(RapidUnmarshalTest, UnserializeHeterogeneousArrayWithOptionalWhenPopulated) {
	std::string json(R"({
						    "response": [
						        {
						            "event": "user_login",
						            "page": "/dashboard", 
						            "duration": 15.75
						        },
						        1672531200,
						        "session_67890"
						    ]
						})");

	OptionalApiResponse apiRes;
	rapidjson_util::unmarshal(json, apiRes);

	const auto& [eventInfo, timeStamp, sessionId] = *apiRes.response;
	ASSERT_EQ(eventInfo.event, "user_login");
	ASSERT_EQ(eventInfo.page, "/dashboard");
	ASSERT_FLOAT_EQ(*eventInfo.duration, 15.75);
	ASSERT_EQ(timeStamp, 1672531200ULL);
	ASSERT_EQ(sessionId, "session_67890");
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

TEST(RAPID_UNMARSHAL_TEST, ThrowsMemberDeserializationExceptionWhenTypeMismatched) {
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

TEST(RAPID_UNMARSHAL_TEST, ThrowWhenJsonArraySizeMismatchesStdFixedArray) {
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

TEST(RAPID_UNMARSHAL_TEST, ThrowWhenJsonArraySizeMismatchesTupleSize) {
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

