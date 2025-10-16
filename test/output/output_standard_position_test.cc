#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_position, flag_format) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance_string{R"JSON({
    "foo": "bar"
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": false
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_position, basic_errors_with_position) {
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

  const auto instance_string{R"JSON({
    "foo": 1
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("keywordLocation"));
  EXPECT_TRUE(error.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(error.defines("instanceLocation"));
  EXPECT_TRUE(error.defines("error"));
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, basic_annotations_with_position) {
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

  const auto instance_string{R"JSON({
    "foo": "bar"
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_EQ(result.at("annotations").size(), 1);

  const auto &annotation{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation.defines("keywordLocation"));
  EXPECT_TRUE(annotation.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(annotation.defines("instanceLocation"));
  EXPECT_TRUE(annotation.defines("annotation"));
  EXPECT_TRUE(annotation.defines("instancePosition"));

  const auto &position{annotation.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, nested_object_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "nested": {
        "properties": {
          "value": { "type": "number" }
        }
      }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance_string{R"JSON({
    "nested": {
      "value": "not a number"
    }
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_GT(errors.size(), 0);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}
