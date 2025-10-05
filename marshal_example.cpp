#include "rapid_util/rapid_util.h"
#include <iostream>

struct Person {
    std::string name;
    int age;
    bool isStudent;
};

// Describe Person members for serialization
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Person, (name, age, isStudent))

void marshal_basic_usage() {
    Person person{ "Alice", 25, true };

    auto json = rapidjson_util::marshal(person);
    std::cout << "=== Basic Usage Example ===" << std::endl;
    std::cout << "JSON encoding of Person object:" << std::endl;
    std::cout << json << "\n" << std::endl;
}

struct Address {
    std::string street;
    std::string city;
    int zipCode;
};

struct Employee {
    std::string name;
    Address address;  // Nested struct
    double salary;
};

// Important: Nested struct Address must be registered first
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Address, (street, city, zipCode))
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Employee, (name, address, salary))

void marshal_nested_structure() {
    Employee employee{
    "John Doe",
    { "123 Main St", "Beijing", 10001 },
        75000.0
    };

    std::string json = rapidjson_util::marshal(employee);

    std::cout << "=== Nested Structure Example ===" << std::endl;
    std::cout << "JSON encoding of Employee with nested Address:" << std::endl;
    std::cout << json << "\n" << std::endl;
}

struct Product {
    std::string productId;
    std::string name;
    double price;
    int quantity;
};

// Register Product for serialization
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Product, (productId, name, price, quantity))

struct Inventory {
    std::string warehouse;
    std::vector<Product> products;  
};

RAPIDJSON_UTIL_DESCRIBE_MEMBERS(Inventory, (warehouse, products))

void marshal_homogeneous_array() {
    Inventory inventory;
    inventory.warehouse = "Main Storage";
    inventory.products = {
        Product{"P1001", "Laptop", 999.99, 50},
        Product{"P1002", "Mouse", 29.99, 200},
        Product{"P1003", "Keyboard", 79.99, 75}
    };

    auto json = rapidjson_util::marshal(inventory);

    std::cout << "=== Homogeneous Array Example ===" << std::endl;
    std::cout << "JSON encoding of Inventory with Product array:" << std::endl;
    std::cout << json << "\n" << std::endl;
}

struct SensorReading {
    std::string sensorType;
    double value;
};

// Register SensorReading for serialization
RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SensorReading, (sensorType, value))

struct SystemStatus {
    std::string timestamp;
    std::tuple<bool, int, SensorReading, std::string> statusData;  
};


RAPIDJSON_UTIL_DESCRIBE_MEMBERS(SystemStatus, (timestamp, statusData))

void marshal_heterogeneous_array() {
    SystemStatus systemStatus;
    systemStatus.timestamp = "2024-01-15T10:30:00Z";
    systemStatus.statusData = std::make_tuple(
        true,
        85,
        SensorReading{ "Temperature", 23.5 },
        "Operational"
    );

    auto json = rapidjson_util::marshal(systemStatus);

    std::cout << "=== Heterogeneous Array Example ===" << std::endl;
    std::cout << "JSON encoding of SystemStatus with mixed-type tuple:" << std::endl;
    std::cout << json << "\n" << std::endl;
}


int main() {
    marshal_basic_usage();          // Simple struct serialization

    marshal_nested_structure();     // Nested object serialization

    marshal_homogeneous_array();    // Array of same-type objects

    marshal_heterogeneous_array();  // Tuple with mixed types
}