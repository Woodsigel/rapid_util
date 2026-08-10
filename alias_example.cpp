/*This example shows how RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS enables
 * custom JSON field name mapping.The __as_is__ placeholder preserves the
 * original member name when no alias is needed.
 */

#include "rapidjs_util/util.h"
#include <iostream>
#include <optional>

struct Address {
    std::string street;
    std::string city;
    std::optional<int> zipCode;
};

struct OrderItem {
    std::string productId;
    int quantity;
    double unitPrice;
};

struct Customer {
    std::string firstName;
    std::string lastName;
    Address address;       
    std::vector<OrderItem> items;   
    bool isActive;
};
 
/**
 * Address member mappings:
 *   street   -> "St"     (remapped)
 *   city     -> "city"   (preserved via __as_is__)
 *   zipCode  -> "ZIP"    (remapped)
 */
RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS(Address, ((street, "St"),            // create alias of street for St
                                                     (city,   __as_is__),       // Preserves "city" as JSON key
                                                     (zipCode, "ZIP")))

/**
 * OrderItem member mappings:
 *   productId -> "id"     (remapped)
 *   quantity  -> "qty"    (remapped)
 *   unitPrice -> "price"  (remapped)
 */
RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS(OrderItem, ((productId, "id"), 
                                                       (quantity, "qty"), 
                                                       (unitPrice, "price")))

/**
 * Customer member mappings:
 *   firstName -> "name"      (remapped)
 *   lastName  -> "surname"   (remapped)
 *   address   -> "addr"      (remapped)
 *   items     -> "items"     (preserved via __as_is__)
 *   isActive  -> "isActive"  (preserved via __as_is__)
 */
RAPIDJSON_UTIL_DESCRIBE_MEMBERS_WITH_ALIAS(Customer, ((firstName, "name"), 
                                                      (lastName, "surname"), 
                                                      (address,  "addr"),
                                                      (items,    __as_is__),   
                                                      (isActive, __as_is__)));


void marshal() {
    Customer customer{
                "John",
                "Doe",
                Address{"Times Square", "New York", std::nullopt},
                {
                    OrderItem{"PROD-001", 2, 29.99},
                    OrderItem{"PROD-002", 1, 99.50},
                    OrderItem{"PROD-003", 3, 14.25}
                },
                true };

    auto json = rapidjson_util::marshal(customer);

    std::cout << "=== Marshal with Member Alias Example ===" << std::endl;
    std::cout << "JSON encoding of customer object:" << std::endl;
    std::cout << json << "\n\n\n" << std::endl;
}

void unmarshal() {
    std::string json = R"({ "name": "Lin",
                            "surname": "Wang",
                            "addr": {
                              "St": "Wangfujing Street",
                              "city": "Beijing",
                              "ZIP": 10001
                            },
                            "items": [
                              { "id": "ID-115", "qty": 1, "price": 89.5   },
                              { "id": "ID-120", "qty": 32, "price": 77.25 }
                            ],
                            "isActive": false
                          })";

    Customer customer;
    rapidjson_util::unmarshal(json, customer);

    std::cout << "=== Unmarshaled with Member Alias Example ===" << std::endl;
    std::cout << "Unmarshaled Customer:" << std::endl;
    std::cout << "  FirstName: " << customer.firstName << std::endl;
    std::cout << "  LastName: " << customer.lastName << std::endl;
    std::cout << "  Address: " << customer.address.street << ", "
        << customer.address.city << ", ";
    auto zipCode = customer.address.zipCode.has_value() ? 
           std::to_string(customer.address.zipCode.value())
           : "null";
    std::cout << zipCode << std::endl;
    std::cout << "  Items:" << std::endl;
    for (const auto& item : customer.items) {
        std::cout << "    - " << item.productId
            << " (Qty: " << item.quantity
            << ", Price: " << item.unitPrice << ")" << std::endl;
    }
    std::cout << "  Is Active: " << (customer.isActive ? "Yes" : "No") << "\n" << std::endl;
}


int main() {
    marshal(); 

    unmarshal(); 
}