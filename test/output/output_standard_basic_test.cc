#include <gtest/gtest.h>

#include <functional>
#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

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

TEST(Output_standard_basic, annotations_include_instance_position) {
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

  sourcemeta::core::PointerPositionTracker positions;
  const auto instance{
      sourcemeta::core::parse_json(R"JSON(5)JSON", std::ref(positions))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  ASSERT_TRUE(result.defines("annotations"));
  ASSERT_TRUE(result.at("annotations").is_array());
  ASSERT_EQ(result.at("annotations").size(), 1);

  const auto &annotation{result.at("annotations").at(0)};
  ASSERT_TRUE(annotation.defines("instanceLocation"));
  ASSERT_TRUE(annotation.defines("instancePosition"));

  const auto location{sourcemeta::core::to_pointer(
      annotation.at("instanceLocation").to_string())};
  const auto expected{positions.get(location)};
  ASSERT_TRUE(expected.has_value());
  EXPECT_EQ(annotation.at("instancePosition"),
            sourcemeta::core::to_json(expected.value()));
}

TEST(Output_standard_basic, errors_include_instance_position) {
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

  sourcemeta::core::PointerPositionTracker positions;
  const auto instance{sourcemeta::core::parse_json(R"JSON({
    "foo": 1
  })JSON",
                                                   std::ref(positions))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  ASSERT_TRUE(result.defines("errors"));
  ASSERT_TRUE(result.at("errors").is_array());
  ASSERT_EQ(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  ASSERT_TRUE(error.defines("instanceLocation"));
  ASSERT_TRUE(error.defines("instancePosition"));

  const auto location{
      sourcemeta::core::to_pointer(error.at("instanceLocation").to_string())};
  const auto expected{positions.get(location)};
  ASSERT_TRUE(expected.has_value());
  EXPECT_EQ(error.at("instancePosition"),
            sourcemeta::core::to_json(expected.value()));
}
