#include <catch2/catch_all.hpp>

#include <iostream>

#include "ImJSchema/detail/json_utils.h"

TEST_CASE("jsonFindPath")
{
    using namespace ImJSchema;
    json J;
    J["grandparent"]["parent"]["child"] = 34;
    J["grandparent"]["array"][4]["child"] = 25;

  //  std::cout << J.dump(4) << std::endl;
    {
        auto p = jsonFindPath("grandparent/parent/child", J);
        REQUIRE(p != nullptr);
        REQUIRE(*p == 34);
    }
    {
        auto p = jsonFindPath("grandparent/parent/sibling", J);
        REQUIRE(p == nullptr);
    }

    // finds an array
    {
        auto p = jsonFindPath("grandparent/array/4/child", J);
        REQUIRE(p != nullptr);
        REQUIRE(*p == 25);
    }
    // we didn't set this value, but it was defaulted
    {
        auto p = jsonFindPath("grandparent/array/3", J);
        REQUIRE(p != nullptr);
        REQUIRE(p->is_null());
    }
    {
        auto p = jsonFindPath("grandparent/array/7", J);
        REQUIRE(p == nullptr);
    }
    {
        auto p = jsonFindPath("grandparent/you/dont/exist/323", J);
        REQUIRE(p == nullptr);
    }
}

TEST_CASE("json merge patch")
{
    using namespace ImJSchema;
    json J;

    auto prop = json::parse(R"foo(
    {
        "name" : "bob",
        "item1": {
            "x" : "hello"
        }
    })foo");

    auto defsRoot = json::parse(R"foo(
    {
        "name" : "bob",
        "lastName" : "Brker",
        "item1": {
            "y" : "world"
        }
    })foo");


    prop.merge_patch(defsRoot);

    std::cout << prop.dump(4) << std::endl;;
}



TEST_CASE("Find Reference list")
{
    using namespace ImJSchema;

    auto J = json::parse(R"foo(
    {
        "$ref" : ["#/$defs/A"]
    })foo");
    auto defs = json::parse(R"foo(
    {
        "$defs" : {
            "A" : {
                "$ref" : ["#/$defs/B", "#/$defs/C"],
                "a_value" : true
            },
            "B" : {
                "$ref" : "#/$defs/D",
                "b_value" : true,
                "d_value" : false
            },
            "C" : {
                "c_value" : true,
                "b_value" : false
            },
            "D" : {
                "d_value" : true
            }
        }
    })foo");

    _jsonExpandReference(J, defs);

    json final;
    final["a_value"] = true;
    final["b_value"] = true;
    final["c_value"] = true;
    final["d_value"] = false;

    std::cout << J.dump(4) << std::endl;;
    REQUIRE( J.dump(4) == final.dump(4));
}

TEST_CASE("jsonExpandAllReferences - check that each type of JSON property can be referenced")
{
    using namespace ImJSchema;
    json J;

    auto prop = json::parse(R"foo(
    {
        "string": {
            "$ref": "#/$defs/string"
        },
        "number": {
            "$ref": "#/$defs/number"
        },
        "boolean": {
            "$ref": "#/$defs/boolean"
        },
        "array": {
            "$ref": "#/$defs/array"
        },
        "object": {
            "$ref": "#/$defs/object"
        }
    })foo");

    auto defsRoot = json::parse(R"foo(
    {
        "$defs" : {
            "string" : "Hello",
            "number" : 3,
            "boolean" : true,
            "array" : [1,2,3],
            "object" : {
                 "test": "hello world"
            }
        }
    })foo");


    jsonExpandAllReferences(prop, defsRoot);

    //std::cout << prop.dump(4);
    {
        REQUIRE(prop.dump(4) == defsRoot["$defs"].dump(4));
    }

}



TEST_CASE("jsonExpandAllReferences - check that each type of JSON property can be referenced asd")
{
    using namespace ImJSchema;
    json J;

    auto prop = json::parse(R"foo(
    {
        "$defs" : {
            "class_list" : ["Wizard", "Sorcerer", "Barbarian"],
            "party_member" : {
                "type" : "object",
                "properties" : {
                    "class" : {
                        "enumNames" : { "$ref" : "#/$defs/class_list" },
                        "enum" : { "$ref" : "#/$defs/class_list" },
                        "type" : "string"
                    }
                }
            },
            "test" : {
                "default" : 1
            }
        },
        "type" : "object",
        "properties" : {
            "member" : {
                "$ref" : "#/$defs/party_member"
            },
            "test2" : {
                "$ref" : "#/$defs/test",
                "default" : 3
            }
        }
    })foo");


    jsonExpandAllReferences(prop, prop);

    REQUIRE( prop["properties"]["test2"]["default"] == 3);
    //std::cout << prop.dump(4);
}

TEST_CASE("setDefaultsRecursive")
{
    auto schema = ImJSchema::json::parse(R"foo(
{
    "type" : "object",
    "properties" : {
        "num" : {
            "default" : 2.0,
            "type" : "number"
        },
        "str" : {
            "default" : "hello",
            "type" : "string"
        },
        "bool" : {
            "default" : true,
            "type" : "boolean"
        },
        "array" : {
            "type" : "array",
            "items" : {
                "type" : "number",
                "default" : 55
            },
            "minItems" : 3
        },
        "object" : {
            "type" : "object",
            "properties" : {
                "value" : {
                    "type" : "string",
                    "default" : "hello world"
                }
            }
        }
    }
}
)foo");

    auto DEFAULTS = ImJSchema::json::parse(R"foo({
    "array": [
        55,
        55,
        55
    ],
    "bool": true,
    "num": 2.0,
    "object": {
        "value": "hello world"
    },
    "str": "hello"
})foo");

    ImJSchema::json value;
    ImJSchema::initializeToDefaults(value, schema);
    std::cout << value.dump(4) << std::endl;

    REQUIRE(value.dump() == DEFAULTS.dump());

    // change the value
    value["array"][2] = 12;
    // increase the size
    value["array"].push_back(3);
    // add a new object
    value["object"]["subobj"] = false;

    // initialize again, will
    // be same is the DEFAULTS
    ImJSchema::initializeToDefaults(value, schema);
    REQUIRE(value.dump() == DEFAULTS.dump());


    // change the value again
    value["array"][2] = 12;
    auto v2 = value;
    // initialize, but this time dont force the reset
    ImJSchema::initializeToDefaults(value, schema, true);
    // value will not change
    REQUIRE(value.dump() == v2.dump());
    REQUIRE(value.dump() != DEFAULTS.dump());
}
