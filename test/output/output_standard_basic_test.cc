#include <gtest/gtest.h>

#include <functional>
#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_basic, prettify_annotations) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const auto instance{sourcemeta::core::parse_json(R"JSON({
    "foo": "bar"
  })JSON")};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{R"JSON({
  "valid": true,
  "annotations": [
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "#/properties",
      "instanceLocation": "",
      "annotation": [ "foo" ]
    }
  ]
})JSON"};

  std::ostringstream prettified;
  sourcemeta::core::prettify(result, prettified);

  EXPECT_EQ(prettified.str(), expected);
}

TEST(Output_standard_basic, prettify_errors) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance{sourcemeta::core::parse_json(R"JSON({
    "foo": 1
  })JSON")};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{R"JSON({
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/foo/type",
      "absoluteKeywordLocation": "#/properties/foo/type",
      "instanceLocation": "/foo",
      "error": "The value was expected to be of type string but it was of type integer"
    }
  ]
})JSON"};

  std::ostringstream prettified;
  sourcemeta::core::prettify(result, prettified);

  EXPECT_EQ(prettified.str(), expected);
}

TEST(Output_standard_basic, success_1) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance{sourcemeta::core::parse_json(R"JSON({
    "foo": "bar"
  })JSON")};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, success_1_exhaustive) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const auto instance{sourcemeta::core::parse_json(R"JSON({
    "foo": "bar"
  })JSON")};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true,
    "annotations": [
      {
        "keywordLocation": "/properties",
        "absoluteKeywordLocation": "#/properties",
        "instanceLocation": "",
        "annotation": [ "foo" ]
      }
    ]
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, success_2) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "anyOf": [
      { "title": "#1", "type": "string" },
      { "title": "#2", "type": "integer" }
    ]
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const sourcemeta::core::JSON instance{5};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, success_2_exhaustive) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "anyOf": [
      { "title": "#1", "type": "string" },
      { "title": "#2", "type": "integer" }
    ]
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const sourcemeta::core::JSON instance{5};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true,
    "annotations": [
      {
        "keywordLocation": "/anyOf/1/title",
        "absoluteKeywordLocation": "#/anyOf/1/title",
        "instanceLocation": "",
        "annotation": [ "#2" ]
      }
    ]
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, success_3) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "not": {
      "title": "Negation",
      "type": "integer"
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const sourcemeta::core::JSON instance{"foo"};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, success_4) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "not": {
      "title": "Negation",
      "type": "integer"
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const sourcemeta::core::JSON instance{"foo"};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, failure_1) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance{sourcemeta::core::parse_json(R"JSON({
    "foo": 1
  })JSON")};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": false,
    "errors": [
      {
        "keywordLocation": "/properties/foo/type",
        "absoluteKeywordLocation": "#/properties/foo/type",
        "instanceLocation": "/foo",
        "error": "The value was expected to be of type string but it was of type integer"
      }
    ]
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, flag_with_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string input{"\"hello\""};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic, errors_with_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string input{R"JSON({
  "foo": 1
})JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error_unit{result.at("errors").at(0)};
  EXPECT_TRUE(error_unit.defines("instancePosition"));
  const auto &position{error_unit.at("instancePosition")};
  EXPECT_EQ(position.size(), 4);
  EXPECT_TRUE(position.at(0).is_integer());
  EXPECT_TRUE(position.at(1).is_integer());
  EXPECT_TRUE(position.at(2).is_integer());
  EXPECT_TRUE(position.at(3).is_integer());
}

TEST(Output_standard_basic, annotations_with_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const std::string input{R"JSON({
  "foo": "bar"
})JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_EQ(result.at("annotations").size(), 1);

  const auto &annotation_unit{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation_unit.defines("instancePosition"));
  const auto &position{annotation_unit.at("instancePosition")};
  EXPECT_EQ(position.size(), 4);
  EXPECT_TRUE(position.at(0).is_integer());
  EXPECT_TRUE(position.at(1).is_integer());
  EXPECT_TRUE(position.at(2).is_integer());
  EXPECT_TRUE(position.at(3).is_integer());
}

TEST(Output_standard_basic, prettify_errors_with_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string input{R"JSON({
  "foo": 1
})JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto expected{R"JSON({
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/foo/type",
      "absoluteKeywordLocation": "#/properties/foo/type",
      "instanceLocation": "/foo",
      "error": "The value was expected to be of type string but it was of type integer",
      "instancePosition": [ 2, 3, 2, 10 ]
    }
  ]
})JSON"};

  std::ostringstream prettified;
  sourcemeta::core::prettify(result, prettified);

  EXPECT_EQ(prettified.str(), expected);
}

TEST(Output_standard_basic, prettify_annotations_with_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const std::string input{R"JSON({
  "foo": "bar"
})JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto expected{R"JSON({
  "valid": true,
  "annotations": [
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "#/properties",
      "instanceLocation": "",
      "annotation": [ "foo" ],
      "instancePosition": [ 1, 1, 3, 1 ]
    }
  ]
})JSON"};

  std::ostringstream prettified;
  sourcemeta::core::prettify(result, prettified);

  EXPECT_EQ(prettified.str(), expected);
}
