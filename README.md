# rapid_util


A simple C++17 based JSON marshal and unmarshal utility for converting between C++ struct objects and JSON using [RapidJSON](https://rapidjson.org) as the underlying parser. The reflection and member description processors in this utility are inspired by from the [Boost Describe library](https://www.boost.org/doc/libs/1_89_0/libs/describe/doc/html/describe.html), with simplifications for ease of use and reduced dependencies.

## Features

-   **Simple API**: Easy-to-use `marshal` and `unmarshal` functions
-   **Nested Structures**: Support for complex object hierarchies
-   **Homogeneous Arrays**: Support JSON homogeneous array serialization handled by `std::vector`, `std::list`, and `std::array`
-   **Heterogeneous Arrays**: Support JSON heterogeneous array serialization handled by `std::tuple`

## Usage

### Basic Serialization
```
#include "rapid_util.h"

struct Person {
    std::string name;
    int age;
    bool isStudent;
};

// Describe struct members for serialization
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Person, (name, age, isStudent))

// Serialize to JSON
Person Alice{"Alice", 25, true};
std::string json = rapidjson_util::marshal(Alice);
// Result: {"name":"Alice","age":25,"isStudent":true}
```

### Basic Deserialization

```
std::string json = R"({
    "name": "Bob",
    "age": 30,
    "isStudent": false
})";

Person bob;
rapidjson_util::unmarshal(json, bob);

std::cout << "Unmarshaled Bob:" << std::endl;
std::cout << "  Name: " << bob.name << std::endl;
std::cout << "  Age: " << bob.age << std::endl;
std::cout << "  Is Student: " << (bob.isStudent ? "Yes" : "No") << "\n" << std::endl;
```
### Nested Structures Serialization
```
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