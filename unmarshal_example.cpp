#include "rapid_util/rapid_util.h"
#include <iostream>

struct Person {
    std::string name;
    int age;
    bool isStudent;
    std::shared_ptr<std::string> email;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Person, (name, age, isStudent, email))

void unmarshal_basic_usage() {
    std::cout << "=== Unmarshal Basic Usage Example ===" << std::endl;

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
    std::cout << "  Email: " << (person.email ? *person.email : "null") << "\n" << std::endl;
}

struct Address {
    std::string street;
    std::string city;
    std::shared_ptr<int> zipCode;
};

struct Employee {
    std::string name;
    Address address;
    double salary;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Address, (street, city, zipCode))
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Employee, (name, address, salary))

void unmarshal_nested_structure() {
    std::cout << "=== Unmarshal Nested Structure Example ===" << std::endl;

    std::string json = R"({
        "name": "Jane Smith",
        "address": {
            "street": "456 Oak Ave",
            "city": "Shanghai",
            "zipCode": null
        },
        "salary": 80000.0
    })";

    Employee employee;
    rapidjson_util::unmarshal(json, employee);

    std::cout << "Unmarshaled Employee:" << std::endl;
    std::cout << "  Name: " << employee.name << std::endl;
    std::cout << "  Address: " << employee.address.street << ", "
        << employee.address.city << ", ";
    auto zipCode = employee.address.zipCode == nullptr ? "null" : std::to_string(*employee.address.zipCode);
    std::cout << zipCode << std::endl;
    std::cout << "  Salary: " << employee.salary << "\n" << std::endl;
}

struct Product {
    std::string productId;
    std::string name;
    double price;
    int quantity;
};

struct Inventory {
    std::string warehouse;
    std::vector<Product> products;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Product, (productId, name, price, quantity))
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Inventory, (warehouse, products))

void unmarshal_homogeneous_array() {
    std::cout << "=== Unmarshal Homogeneous Array Example ===" << std::endl;

    std::string json = R"({
        "warehouse": "East Storage",
        "products": [
            {"productId": "E1001", "name": "Monitor", "price": 299.99, "quantity": 25},
            {"productId": "E1002", "name": "Webcam", "price": 49.99, "quantity": 100},
            {"productId": "E1003", "name": "Headphones", "price": 89.99, "quantity": 60}
        ]
    })";

    Inventory inventory;
    rapidjson_util::unmarshal(json, inventory);

    std::cout << "Unmarshaled Inventory:" << std::endl;
    std::cout << "  Warehouse: " << inventory.warehouse << std::endl;
    std::cout << "  Products:" << std::endl;
    for (const auto& product : inventory.products) {
        std::cout << "    - " << product.productId << ": " << product.name
            << " ($" << product.price << ", Qty: " << product.quantity << ")" << std::endl;
    }
    std::cout << std::endl;
}

struct SensorReading {
    std::string sensorType;
    double value;
};

struct SystemStatus {
    std::string timestamp;
    std::tuple<bool, int, SensorReading, std::string> statusData;
    std::shared_ptr<std::tuple<double, std::string, int>> diagnostics;
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SensorReading, (sensorType, value))
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SystemStatus, (timestamp, statusData, diagnostics))

void unmarshal_heterogeneous_array() {
    std::cout << "=== Unmarshal Heterogeneous Array Example ===" << std::endl;

    std::string json = R"({
        "timestamp": "2024-01-16T14:45:00Z",
        "statusData": [false, 42, {"sensorType": "Humidity", "value": 65.2}, "Maintenance"],
        "diagnostics": null
    })";

    SystemStatus systemStatus;
    rapidjson_util::unmarshal(json, systemStatus);

    const auto& [isOnline, sensorCount, reading, status] = systemStatus.statusData;

    std::cout << "Unmarshaled System Status:" << std::endl;
    std::cout << "  Timestamp: " << systemStatus.timestamp << std::endl;
    std::cout << "  Status Data:" << std::endl;
    std::cout << "    - Online: " << (isOnline ? "Yes" : "No") << std::endl;
    std::cout << "    - Sensor Count: " << sensorCount << std::endl;
    std::cout << "    - Sensor Reading: " << reading.sensorType << " = " << reading.value << std::endl;
    std::cout << "    - Status: " << status << "\n" << std::endl;

    if (systemStatus.diagnostics) {
        const auto& [uptime, health, operations] = *systemStatus.diagnostics;
        std::cout << "  Diagnostics:" << std::endl;
        std::cout << "    - Uptime: " << uptime << "%" << std::endl;
        std::cout << "    - Health: " << health << std::endl;
        std::cout << "    - Operations: " << operations << std::endl;
    }
    else {
        std::cout << "  Diagnostics: None (null)" << std::endl;
    }
}


int main() {
    unmarshal_basic_usage();           // Simple struct deserialization

    unmarshal_nested_structure();      //  Nested object serialization

    unmarshal_homogeneous_array();     // Array of same-type objects

    unmarshal_heterogeneous_array();   // Tuple with mixed types
}