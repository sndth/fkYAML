//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library (supporting code)
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.5.0
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

#include <map>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include <fkYAML/node.hpp>

TEST_CASE("Document_Access") {
    const fkyaml::document doc = fkyaml::document::parse("name: fkYAML\n"
                                                         "version: 5\n"
                                                         "ratio: 0.5\n"
                                                         "stable: true\n"
                                                         "nothing: null\n"
                                                         "'quoted': \"line\\nbreak\"\n"
                                                         "text: |\n"
                                                         "  block\n"
                                                         "list: [1, two, 3.0]\n");

    REQUIRE(doc.root().is_mapping());
    REQUIRE(doc.root().size() == 8);
    REQUIRE(doc["name"].is_string());
    REQUIRE(doc["name"].as_str() == "fkYAML");
    REQUIRE(doc["name"].as<std::string>() == "fkYAML");
    REQUIRE(doc["version"].as<int>() == 5);
    REQUIRE(doc["ratio"].as<double>() == 0.5);
    REQUIRE(doc["stable"].as<bool>() == true);
    REQUIRE(doc["nothing"].is_null());
    REQUIRE(doc["quoted"].as_str() == "line\nbreak");
    REQUIRE(doc["text"].as_str() == "block\n");
    REQUIRE(doc["list"].size() == 3);
    REQUIRE(doc["list"][0].as<int>() == 1);
    REQUIRE(doc["list"][1].as_str() == "two");
    REQUIRE(doc["list"][2].as<float>() == 3.f);

    REQUIRE(doc.root().contains("name"));
    REQUIRE_FALSE(doc.root().contains("missing"));
    REQUIRE_THROWS_AS(doc["missing"], fkyaml::exception);
    REQUIRE_THROWS_AS(doc["list"][3], fkyaml::exception);
    REQUIRE_THROWS_AS(doc["list"]["key"], fkyaml::type_error);
    REQUIRE_THROWS_AS(doc["name"][0], fkyaml::type_error);
    REQUIRE_THROWS_AS(doc["list"].as_str(), fkyaml::type_error);
    REQUIRE_THROWS_AS(doc["version"].key()[0], fkyaml::type_error);
    REQUIRE_THROWS_AS(doc.root().key(), fkyaml::exception);
}

TEST_CASE("Document_Iteration") {
    const fkyaml::document doc = fkyaml::document::parse("b: 1\na: 2\nc: [x, y]\n");

    std::vector<std::string> keys {};
    for (const fkyaml::document_node value : doc.root()) {
        keys.emplace_back(value.key().as<std::string>());
    }
    // Keys stay in the order of the input.
    REQUIRE(keys == std::vector<std::string> {"b", "a", "c"});

    std::string joined {};
    for (const fkyaml::document_node entry : doc["c"]) {
        joined += entry.as<std::string>();
    }
    REQUIRE(joined == "xy");
}

TEST_CASE("Document_ConversionsThroughNode") {
    const fkyaml::document doc = fkyaml::document::parse("list: [1, 2]\nmap: {a: 1}\ninteger: 1\n");
    REQUIRE(doc["list"].as<std::vector<int>>() == std::vector<int> {1, 2});
    REQUIRE(doc["map"].as<std::map<std::string, int>>() == std::map<std::string, int> {{"a", 1}});
    // The same conversion as basic_node::get_value() does.
    REQUIRE(doc["integer"].as<float>() == 1.f);
    REQUIRE_THROWS_AS(doc["list"].as<int>(), fkyaml::exception);
}

TEST_CASE("Document_AnchorsAndAliases") {
    const fkyaml::document doc = fkyaml::document::parse("a: &x {k: v}\n"
                                                         "b: *x\n"
                                                         "c: &y !!str 1\n"
                                                         "d: *y\n"
                                                         "e: &x 2\n"
                                                         "f: *x\n");
    REQUIRE(doc["a"].is_anchor());
    REQUIRE(doc["a"].get_anchor_name() == "x");
    REQUIRE(doc["b"].is_alias());
    REQUIRE(doc["b"].get_anchor_name() == "x");
    REQUIRE(doc["b"]["k"].as_str() == "v");
    REQUIRE(doc["d"].is_string());
    REQUIRE(doc["d"].has_tag_name());
    REQUIRE(doc["f"].as<int>() == 2);

    REQUIRE_THROWS_AS(fkyaml::document::parse("&x [*x]"), fkyaml::parse_error);
    REQUIRE_THROWS_AS(fkyaml::document::parse("*unknown"), fkyaml::parse_error);
    REQUIRE_NOTHROW(fkyaml::document::parse("foo: &x\n  bar: 1\n*x : 2\n"));
}

TEST_CASE("Document_DuplicateKeys") {
    auto input = GENERATE(
        std::string("a: 1\na: 2\n"),
        std::string("a: 1\n\"a\": 2\n"),
        std::string("1: a\n0x1: b\n"),
        std::string("1.0: a\n1.00: b\n"),
        std::string("true: a\ntrue: b\n"),
        std::string("~: a\nnull: b\n"),
        std::string("[1, 2]: a\n[1, 2]: b\n"),
        std::string("{a: 1, b: 2}: a\n{b: 2, a: 1}: b\n"));
    REQUIRE_THROWS_AS(fkyaml::document::parse(input), fkyaml::parse_error);
    REQUIRE_THROWS_AS(fkyaml::node::deserialize(input), fkyaml::parse_error);

    std::string large {};
    for (int i = 0; i < 1000; i++) {
        large += "key" + std::to_string(i) + ": 1\n";
    }
    REQUIRE_NOTHROW(fkyaml::document::parse(large));
    REQUIRE_THROWS_AS(fkyaml::document::parse(large + "key500: 2\n"), fkyaml::parse_error);
    REQUIRE_THROWS_AS(fkyaml::document::parse(large + "'key999': 2\n"), fkyaml::parse_error);

    std::string large_integers {};
    for (int i = 0; i < 100; i++) {
        large_integers += std::to_string(i) + ": 1\n";
    }
    REQUIRE_THROWS_AS(fkyaml::document::parse(large_integers + "0x10: 2\n"), fkyaml::parse_error);
}

TEST_CASE("Document_TaggedScalars") {
    const fkyaml::document doc = fkyaml::document::parse("a: !!str 123\nb: !!int \"123\"\nc: !custom 1\n");
    REQUIRE(doc["a"].is_string());
    REQUIRE(doc["b"].is_integer());
    REQUIRE(doc["b"].as<int>() == 123);
    REQUIRE(doc["c"].is_integer());
    REQUIRE(doc["c"].get_tag_name() == "!custom");

    REQUIRE_THROWS_AS(fkyaml::document::parse("!!int abc"), fkyaml::parse_error);
}

// Inputs which call for an operation on a node of the wrong type must be rejected rather than crash.
TEST_CASE("Document_RejectsWhatDeserializeRejects") {
    auto input = GENERATE(
        std::string("- a\n- b\nc: d\n"),
        std::string("a: b\n- c\n"),
        std::string("{a: 1\n"),
        std::string("a: [1, 2\n"),
        std::string("- &x [1\n"),
        std::string("foo: bar: baz\n"));
    REQUIRE_THROWS_AS(fkyaml::node::deserialize(input), fkyaml::exception);
    REQUIRE_THROWS_AS(fkyaml::document::parse(input), fkyaml::exception);
}

TEST_CASE("Document_OutOfRangeIntegersAreStrings") {
    const fkyaml::document doc = fkyaml::document::parse("a: 99999999999999999999999\nb: 18446744073709551615\n");
    REQUIRE(doc["a"].is_string());
    REQUIRE(doc["b"].is_integer());
    REQUIRE(doc.to_node() == fkyaml::node::deserialize("a: 99999999999999999999999\nb: 18446744073709551615\n"));
}

TEST_CASE("Document_MultipleDocuments") {
    const std::vector<fkyaml::document> docs =
        fkyaml::document::parse_docs("%YAML 1.2\n---\nfoo: 1\n---\n- bar\n...\n");
    REQUIRE(docs.size() == 2);
    REQUIRE(docs[0]["foo"].as<int>() == 1);
    REQUIRE(docs[1][0].as_str() == "bar");
    REQUIRE(docs[0].to_node().get_yaml_version_type() == fkyaml::yaml_version_type::VERSION_1_2);
}

TEST_CASE("Document_OutlivesItsInput") {
    fkyaml::document doc = fkyaml::document::parse("placeholder: 1\n");
    {
        const std::string input = "key: value\n";
        doc = fkyaml::document::parse(input);
    }
    const fkyaml::document copy = doc;
    REQUIRE(copy["key"].as_str() == "value");
}

TEST_CASE("Document_InputTypes") {
    const std::string input = "foo: bar\r\nbaz: 1\r\n";
    REQUIRE(fkyaml::document::parse(input)["foo"].as_str() == "bar");
    REQUIRE(fkyaml::document::parse(input.begin(), input.end())["baz"].as<int>() == 1);
    REQUIRE(fkyaml::document::parse("baz: 1\n")["baz"].as<int>() == 1);
}

// Everything which parses into a document must convert to what deserializing the same input gives.
TEST_CASE("Document_ToNodeMatchesDeserialize") {
    auto input = GENERATE(
        std::string(""),
        std::string("---\n"),
        std::string("plain"),
        std::string("\"quoted\""),
        std::string("- a\n- b: 1\n  c: [1, {d: e}]\n"),
        std::string("? complex\n: value\n? [1, 2]\n: {a: b}\n"),
        std::string("a: &x [1, 2]\nb: *x\nz: &x 3\nc: *x\n"),
        std::string("&root\na: !!str 1\nb: !custom {c: d}\n"),
        std::string("%YAML 1.1\n%TAG ! tag:example.com,2000:\n---\na: !foo bar\n"),
        std::string("text: >\n  folded\n  lines\n\nother: |-\n  literal\n"),
        std::string("multi: first\n  second\n  third\nnext: 'it''s'\n"),
        std::string("[a, b: c, {d: e}, ]"),
        std::string("0x10: 0o7\n1e3: 0.5\n~: null\n"),
        std::string("a: !!int #comment\n  not an integer\n"));

    const fkyaml::node expected = fkyaml::node::deserialize(input);
    const fkyaml::node actual = fkyaml::document::parse(input).to_node();
    REQUIRE(actual == expected);
    REQUIRE(fkyaml::node::serialize(actual) == fkyaml::node::serialize(expected));
}
