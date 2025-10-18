#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>
#include "gmock/gmock.h"

#include "rapid_util/rapid_util.h"


std::string removeWhitespaceOutsideQuotes(const std::string& input) {
	std::regex pattern(R"(\s+(?=(?:[^"]*"[^"]*")*[^"]*$))");

	return std::regex_replace(input, pattern, "");
}

/**
 * @brief Truncates decimal numbers in a string input to specified precision
 */
std::string truncateDecimals(const std::string& input, int precision) {
	std::regex decimalRegex(R"((\d+)\.(\d+))");
	std::string result;
	std::sregex_iterator it(input.begin(), input.end(), decimalRegex);
	std::sregex_iterator end;

	size_t lastPos = 0;

	for (; it != end; ++it) {
		std::smatch match = *it;

		auto pp = match.position() - lastPos;
		// Append the part of the string before the regex match
		result += input.substr(lastPos, match.position() - lastPos);

		std::string integerPart = match[1].str();
		std::string fractionalPart = match[2].str();

		// Truncate the fractional part to the specified precision
		if (precision < fractionalPart.length()) {
			fractionalPart = fractionalPart.substr(0, precision);
		}

		result += integerPart + "." + fractionalPart;

		lastPos = match.position() + match[0].str().size();
	}

	// Append the remaining part of the input string
	result += input.substr(lastPos);

	return result;
}


TEST(MARSHAL_TEST_UTIL, RemoveWhitespaceOutsideQuotes) {
	std::string json = R"({  "city" : "New York" })";

	std::string exepct = R"({"city":"New York"})";

	ASSERT_STREQ(removeWhitespaceOutsideQuotes(json).c_str(), exepct.c_str());
}

TEST(MARSHAL_TEST_UTIL, TruncateDecimals) {
	int precision = 1;
	auto actual = truncateDecimals(" 9.424987, 84, .213, 123.312f, 74.f ", precision);
	std::string expect{ " 9.4, 84, .213, 123.3f, 74.f " };

	ASSERT_STREQ(actual.c_str(), expect.c_str());
}

#define ASSERT_JSON_STREQ(actual, expect) \
    ASSERT_STREQ(actual.c_str(), removeWhitespaceOutsideQuotes(expect).c_str())

struct AbitraryStruct {
	int IntNumber;
	int64_t Int64Number;
	uint64_t Uint64Number;
	bool BoolValue;
	float FloatNumber;
	double DoubleNumber;
	std::string Str;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(AbitraryStruct, (IntNumber, Int64Number, Uint64Number, BoolValue, FloatNumber, DoubleNumber, Str))

TEST(RapidMarshalTest, SerializePrimitiveTypes) {
	AbitraryStruct s;
	s.IntNumber = 42;
	s.Int64Number = -9876543210LL;
	s.Uint64Number = 18446744073709551615ULL;
	s.BoolValue = true;
	s.FloatNumber = 3.14f;
	s.DoubleNumber = 2.76;
	s.Str = "Hello";

	auto actual = rapidjson_util::marshal(s);

	auto expect = R"({
                       "IntNumber":42,
                       "Int64Number" : -9876543210,
                       "Uint64Number" : 18446744073709551615,
                       "BoolValue" : true,
                       "FloatNumber" : 3.14,
                       "DoubleNumber" : 2.76,
                       "Str" : "Hello"
                      })";

	ASSERT_JSON_STREQ(truncateDecimals(actual, 2), expect);
}

struct NullableFieldsWithOptional {
	std::optional<int> IntNumber;
	std::optional<int64_t> Int64Number;
	std::optional<uint64_t> Uint64Number;
	std::optional<bool> Bool;
	std::optional<float> FloatNumber;
	std::optional<double> DoubleNumber;
	std::optional<std::string> Str;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(NullableFieldsWithOptional, (IntNumber, Int64Number, Uint64Number, Bool, FloatNumber, DoubleNumber, Str))

TEST(RapidUnmarshalTest, SerializeNullablePrimitiveTypesWithOptionalWhenNull) {
	NullableFieldsWithOptional  f;

	auto actual = rapidjson_util::marshal(f);

	auto expect = R"({
						"IntNumber"    : null, 
						"Int64Number"  : null,
						"Uint64Number" : null, 
						"Bool" : null, 
						"FloatNumber"  : null, 
						"DoubleNumber" : null,
						"Str" : null
                     })";

	ASSERT_JSON_STREQ(actual, expect);
}

TEST(RapidUnmarshalTest, SerializeNullablePrimitiveTypesWithOptionalWhenPopulated) {
	NullableFieldsWithOptional  f;

	f.IntNumber = 66;
	f.Int64Number = 4137901254LL;
	f.Uint64Number = 5843644404370511615ULL;
	f.Bool = false;
	f.FloatNumber = 94.887f;
	f.DoubleNumber = 50.241;
	f.Str = "Str";

	auto actual = rapidjson_util::marshal(f);

	auto expect = R"({
						"IntNumber"    : 66, 
						"Int64Number"  : 4137901254,
						"Uint64Number" : 5843644404370511615, 
						"Bool"         : false, 
						"FloatNumber"  : 94.887, 
						"DoubleNumber" : 50.241,
						"Str"          : "Str"
                     })";

	ASSERT_JSON_STREQ(truncateDecimals(actual, 3), expect);
}

struct Address {
	std::string street;
	std::string city;
	int zipCode;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Address, (street, city, zipCode))

struct Person {
	int age;
	bool isMarried;
	Address addr;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Person, (age, isMarried, addr))

TEST(RapidMarshalTest, SerializeNestedStruct) {
	Person person;
	person.age = 23;
	person.isMarried = false;
	person.addr.street = "123 Main St";
	person.addr.city = "Beijing";
	person.addr.zipCode = 65001;

	auto actual = rapidjson_util::marshal(person);

	auto expect = R"({ 
                       "age" : 23,
					   "isMarried" : false,
                       "addr" : {
                                   "street" : "123 Main St",
                                   "city" : "Beijing",
                                   "zipCode" : 65001
                                 }
                       })";

	ASSERT_JSON_STREQ(actual, expect);
}


struct Author {
	std::string name;
	std::string nationality;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Author, (name, nationality))

struct Book {
	std::string title;
	std::optional<Author> author;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Book, (title, author))

TEST(RapidUnmarshalTest, SerializeNestedStructWithOptionalWhenNull) {
	Book book{ "Classic of Poetry", std::nullopt };

	auto actual = rapidjson_util::marshal(book);

	auto expect = R"({ 
                       "title" : "Classic of Poetry",
					   "author" : null
                       })";

	ASSERT_JSON_STREQ(actual, expect);
}


TEST(RapidUnmarshalTest, SerializeNestedStructWithOptionalWhenPopulated) {
	Book book{ "The Nine Chapters on the Mathematical Art", Author{"Liu Hui", "China"} };

	auto actual = rapidjson_util::marshal(book);

	auto expect = R"({ 
                       "title" : "The Nine Chapters on the Mathematical Art",
					   "author" : {
									"name" : "Liu Hui",
								    "nationality" : "China"
								  }
                       })";

	ASSERT_JSON_STREQ(actual, expect);
}


struct Course {
	std::string courseCode;
	std::string courseName;
	std::string grade;
	int credits;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Course, (courseCode, courseName, grade, credits))

struct Student {
	int studentId;
	std::list<Course> enrolledCourses;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Student, (studentId, enrolledCourses))

TEST(RapidMarshalTest, SerializeHomogeneousArray) {
	Student student;
	student.studentId = 1;
	student.enrolledCourses.emplace_back(Course{ "MATH101", "Calculus", "A+", 3});
	student.enrolledCourses.emplace_back(Course{ "MATH203", "Algebra II", "A+", 4 });
	student.enrolledCourses.emplace_back(Course{ "ENG301", "Literature", "C+", 3 });

	auto actual = rapidjson_util::marshal(student);

	auto expect = R"({
						 "studentId" : 1,
						 "enrolledCourses" : 
						 		[{"courseCode":"MATH101","courseName":"Calculus","grade":"A+","credits":3},
						 		 {"courseCode":"MATH203","courseName":"Algebra II","grade":"A+","credits":4},
						 		 {"courseCode":"ENG301","courseName":"Literature","grade":"C+","credits":3}]
					  })";

	ASSERT_JSON_STREQ(actual, expect);
}

struct StudentWithOptionalCourseList {
	int studentId;
	std::optional<std::list<Course>> enrolledCourses;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(StudentWithOptionalCourseList, (studentId, enrolledCourses))

TEST(RapidUnmarshalTest, SerializeHomogeneousArrayWithOptionalWhenNull) {
	StudentWithOptionalCourseList student;

	student.studentId = 100;
	student.enrolledCourses = std::nullopt;

	auto actual = rapidjson_util::marshal(student);

	auto expect = R"({
						 "studentId" : 100,
						 "enrolledCourses" : null
					  })";

	ASSERT_JSON_STREQ(actual, expect);
}

TEST(RapidUnmarshalTest, SerializeHomogeneousArrayWithOptionalWhenEmpty) {
	StudentWithOptionalCourseList student;

	student.studentId = 200;
	student.enrolledCourses = std::list<Course>{};

	auto actual = rapidjson_util::marshal(student);

	auto expect = R"({
						 "studentId" : 200,
						 "enrolledCourses" : []
					  })";

	ASSERT_JSON_STREQ(actual, expect);
}

TEST(RapidUnmarshalTest, SerializeHomogeneousArrayWithOptionalWhenPopulated) {
	StudentWithOptionalCourseList student;
	student.studentId = 300;
	student.enrolledCourses = std::list<Course>{};

	student.enrolledCourses->emplace_back(Course{ "CS101", "Introduction to Computer Science", "B+", 3 });
	student.enrolledCourses->emplace_back(Course{ "CHEM115", "General Chemistry I", "C+", 4 });
	student.enrolledCourses->emplace_back(Course{ "ENG150", "Shakespeare's Major Works", "A-", 3 });

	auto actual = rapidjson_util::marshal(student);

	auto expect = R"({
						 "studentId" : 300,
						 "enrolledCourses" : 
						 		[{"courseCode":"CS101",   "courseName":"Introduction to Computer Science","grade":"B+","credits":3},
						 		 {"courseCode":"CHEM115", "courseName":"General Chemistry I",             "grade":"C+","credits":4},
						 		 {"courseCode":"ENG150",  "courseName":"Shakespeare's Major Works",       "grade":"A-","credits":3}]
					  })";

	ASSERT_JSON_STREQ(actual, expect);
}


struct StudentWithOptionalCourseElements {
	int studentId;
	std::vector<std::optional<Course>> enrolledCourses;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(StudentWithOptionalCourseElements, (studentId, enrolledCourses))


TEST(RapidUnmarshalTest, SerializeHomogeneousArrayWithOptionalWhenContainNulls) {
	StudentWithOptionalCourseElements student;
	student.studentId = 400;
	student.enrolledCourses.emplace_back(std::nullopt);
	student.enrolledCourses.emplace_back(Course{ "CS101", "Introduction to Computer Science", "B+", 3 });
	student.enrolledCourses.emplace_back(std::nullopt);
	student.enrolledCourses.emplace_back(Course{ "ENG150", "Shakespeare's Major Works", "A-", 3 });

	auto actual = rapidjson_util::marshal(student);

	auto expect = R"({
						 "studentId" : 400,
						 "enrolledCourses" : 
						 			 [null,
									 {"courseCode":"CS101",   "courseName":"Introduction to Computer Science","grade":"B+","credits":3},
						 			 null,
						 			 {"courseCode":"ENG150",  "courseName":"Shakespeare's Major Works",       "grade":"A-","credits":3}]
					  })";

	ASSERT_JSON_STREQ(actual, expect);
}

struct User {
	int id;
	std::string name;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(User, (id, name))


struct Response {
	std::string header;
	std::tuple<std::string, int, User> content;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Response, (header, content))

TEST(RapidMarshalTest, SerializeHeterogeneousArray) {
	Response response;
	response.header = "/101/Forbiden";
	response.content = std::make_tuple("success", 200, User{ 10, "John" });


	auto actual = rapidjson_util::marshal(response);

	auto expect = R"({
                      "header" : "/101/Forbiden",
                      "content" : ["success", 200, {"id" : 10,"name" : "John"}]
                    })";

	ASSERT_JSON_STREQ(actual, expect);
}

struct ResponseWithOptionalContent {
	std::string header;
	std::optional<std::tuple<std::string, int, User>> content;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(ResponseWithOptionalContent, (header, content))

TEST(RapidMarshalTest, SerializeHeterogeneousArrayWithOptionalWhenNull) {
	ResponseWithOptionalContent response;
	response.header = "500/Internal Server Error";
	response.content = std::nullopt;


	auto actual = rapidjson_util::marshal(response);

	auto expect = R"({
                      "header": "500/Internal Server Error",
                      "content" : null
                    })";

	ASSERT_JSON_STREQ(actual, expect);
}

TEST(RapidMarshalTest, SerializeHeterogeneousArrayWithOptionalWhenPopulated) {
	ResponseWithOptionalContent response;
	response.header = "/404/Not Found";
	response.content = std::make_tuple("failure", 500, User{ 85, "Wu" });


	auto actual = rapidjson_util::marshal(response);

	auto expect = R"({
                      "header": "/404/Not Found",
                      "content" : ["failure", 500, {"id" : 85, "name" : "Wu"}]
                    })";

	ASSERT_JSON_STREQ(actual, expect);
}