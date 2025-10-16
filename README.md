
# rapid_util


A simple C++17 based JSON marshal and unmarshal utility for converting between C++ struct objects and JSON using [RapidJSON](https://rapidjson.org) as the underlying parser. The reflection preprocessors in this utility are inspired by [Boost Describe library](https://www.boost.org/doc/libs/1_89_0/libs/describe/doc/html/describe.html), with simplifications for ease of use and reduced dependencies.

## Features

- **Simple API**: Easy-to-use `marshal` and `unmarshal` functions
- **Nested Structures**: Support for complex object hierarchies
- **Homogeneous Arrays**: Support for JSON homogeneous array serialization using `std::vector`, `std::list`, and `std::array`
- **Heterogeneous Arrays**: Support for JSON heterogeneous array serialization using `std::tuple`
- **JSON Null Values**: Support for JSON null values using `std::shared_ptr`

## Usage

### Basic Serialization
```
#include "rapid_util.h"

struct Person {
    std::string name;
    int age;
    bool isStudent;
    std::shared_ptr<std::string> email;
};

// Describe Person members for serialization
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Person, (name, age, isStudent, email)))

// Serialize to JSON
Person Alice{"Alice", 25, true, nullptr};
std::string json = rapidjson_util::marshal(Alice);
// Result: {"name":"Alice","age":25,"isStudent":true,"email":null}
```

### Basic Deserialization

```
std::string json = R"({
    "name": "Bob",
    "age": 30,
    "isStudent": false,
    "email" : null
})";

Person person;
rapidjson_util::unmarshal(json, person);

std::cout << "Unmarshaled Person:" << std::endl;
std::cout << "  Name: " << person.name << std::endl;
std::cout << "  Age: " << person.age << std::endl;
std::cout << "  Is Student: " << (person.isStudent ? "Yes" : "No")  << std::endl;
std::cout << "  Email: " << (person.email ? *person.email : "null") << std::endl;
```
### Nested Structures Serialization
```
#include "rapid_util.h"

// Serialize to JSON

struct Address {
    std::string street;
    std::string city;
    int zipCode;
};

struct Employee {
    std::string name;
    Address address;
    double salary;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Address, (street, city, zipCode))
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Employee, (name, address, salary))

Employee employee{
    "John Doe", 
    {"123 Main St", "New York", 10001}, 
    75000.0
};


std::string json = rapidjson_util::marshal(employee);
// Result: {"name":"John Doe","address":{"street":"123 Main St","city":"New York","zipCode":10001},"salary":75000.0}
```
### Nested Structures Deserialization
```
#include "rapid_util.h"
#include <cassert>

// Deserialize from JSON  

json = R"({
    "name": "Jane Smith",
    "address": {
        "street": "456 Oak Ave",
        "city": "Shanghai",
        "zipCode": 200000
    },
    "salary": 80000.0
})";

Employee employee;
rapidjson_util::unmarshal(json, employee);

std::cout << "Unmarshaled Employee:" << std::endl;
std::cout << "  Name: " << employee.name << std::endl;
std::cout << "  Address: " << employee.address.street << ", "
    << employee.address.city << ", " << employee.address.zipCode << std::endl;
std::cout << "  Salary: " << employee.salary << "\n" << std::endl;

struct DatabaseConfig {
	std::string host;
	int port;
	std::shared_ptr<Credential> credential;  // Optional nested object
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(DatabaseConfig, (host, port, credential))

auto json = R"( {
				"host": "localhost",
				"port": 4212,
				"credential": null
				})";

DatabaseConfig config;
rapidjson_util::unmarshal(json, config);

std::cout << "Unmarshaled DatabaseConfig:" << std::endl;
std::cout << "  Host: " << config.host << std::endl;
std::cout << "  Port: " << config.port << std::endl;
assert(config.credential == nullptr);
```

## Installation
`rapid_util` is header-only. Just copy `rapid_util/include` to your project's include path. It requires the RapidJSON library for its parsing functionality, please ensure RapidJSON is available in your include path

## Requirements
-   C++17 or later
-   RapidJSON library

# 中文简介

rapid_util 是一个基于RapidJSON的简易C++17工具，用于实现C++结构体与JSON之间的序列化与反序列化。其反射预处理器借鉴了Boost Describe库的设计，但进行了简化以降低使用难度并减少依赖项。可以满足大多数 JSON 序列化应用场景的需求。

Author: Liu Wu
Mail: woodsliu856@gmail.com