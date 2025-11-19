#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_position, basic_with_position_annotations) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_string{R"JSON({
    "foo": "bar"
  })JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  const auto schema{sourcemeta::core::parse_json(schema_string)};
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(positions))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());

  const auto &annotations{result.at("annotations").as_array()};
  EXPECT_GT(annotations.size(), 0);

  for (const auto &annotation : annotations) {
    EXPECT_TRUE(annotation.is_object());
    EXPECT_TRUE(annotation.defines("keywordLocation"));
    EXPECT_TRUE(annotation.defines("absoluteKeywordLocation"));
    EXPECT_TRUE(annotation.defines("instanceLocation"));
    EXPECT_TRUE(annotation.defines("annotation"));
  }
}

TEST(Output_standard_position, basic_with_position_errors) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_string{R"JSON({
    "foo": 1
  })JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  const auto schema{sourcemeta::core::parse_json(schema_string)};
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(positions))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());

  const auto &errors{result.at("errors").as_array()};
  EXPECT_GT(errors.size(), 0);

  for (const auto &error : errors) {
    EXPECT_TRUE(error.is_object());
    EXPECT_TRUE(error.defines("keywordLocation"));
    EXPECT_TRUE(error.defines("absoluteKeywordLocation"));
    EXPECT_TRUE(error.defines("instanceLocation"));
    EXPECT_TRUE(error.defines("error"));

    if (error.defines("instancePosition")) {
      EXPECT_TRUE(error.at("instancePosition").is_array());
      EXPECT_EQ(error.at("instancePosition").size(), 4);
    }
  }
}

TEST(Output_standard_position, flag_with_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_string{R"JSON({
    "foo": "bar"
  })JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  const auto schema{sourcemeta::core::parse_json(schema_string)};
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(positions))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, positions)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_position, nested_object_with_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "nested": {
        "properties": {
          "value": { "type": "number" }
        }
      }
    }
  })JSON"};

  const auto instance_string{R"JSON({
    "nested": {
      "value": "not a number"
    }
  })JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  const auto schema{sourcemeta::core::parse_json(schema_string)};
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(positions))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors").as_array()};
  EXPECT_GT(errors.size(), 0);

  for (const auto &error : errors) {
    EXPECT_TRUE(error.is_object());
    if (error.defines("instancePosition")) {
      const auto &position{error.at("instancePosition")};
      EXPECT_TRUE(position.is_array());
      EXPECT_EQ(position.size(), 4);
      EXPECT_TRUE(position.at(0).is_integer());
      EXPECT_TRUE(position.at(1).is_integer());
      EXPECT_TRUE(position.at(2).is_integer());
      EXPECT_TRUE(position.at(3).is_integer());
    }
  }
}
