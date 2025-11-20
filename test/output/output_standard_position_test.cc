#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_position, basic_annotations_with_positions) {
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

  const auto instance_text{R"JSON({
    "foo": "bar"
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());

  const auto &annotations{result.at("annotations")};
  EXPECT_GT(annotations.size(), 0);

  const auto &first_annotation{annotations.at(0)};
  EXPECT_TRUE(first_annotation.defines("keywordLocation"));
  EXPECT_TRUE(first_annotation.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(first_annotation.defines("instanceLocation"));
  EXPECT_TRUE(first_annotation.defines("annotation"));
  EXPECT_TRUE(first_annotation.defines("instancePosition"));

  const auto &position{first_annotation.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, basic_errors_with_positions) {
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

  const auto instance_text{R"JSON({
    "foo": 1
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());

  const auto &errors{result.at("errors")};
  EXPECT_GT(errors.size(), 0);

  const auto &first_error{errors.at(0)};
  EXPECT_TRUE(first_error.defines("keywordLocation"));
  EXPECT_TRUE(first_error.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(first_error.defines("instanceLocation"));
  EXPECT_TRUE(first_error.defines("error"));
  EXPECT_TRUE(first_error.defines("instancePosition"));

  const auto &position{first_error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);

  EXPECT_TRUE(position.at(0).is_integer());
  EXPECT_TRUE(position.at(1).is_integer());
  EXPECT_TRUE(position.at(2).is_integer());
  EXPECT_TRUE(position.at(3).is_integer());
}

TEST(Output_standard_position, flag_format_no_positions) {
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

  const auto instance_text{R"JSON({
    "foo": "bar"
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_FALSE(result.defines("annotations"));
  EXPECT_FALSE(result.defines("errors"));
}

TEST(Output_standard_position, nested_property_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "nested": {
        "properties": {
          "value": { "type": "string" }
        }
      }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance_text{R"JSON({
    "nested": {
      "value": 42
    }
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors")};
  EXPECT_GT(errors.size(), 0);

  const auto &first_error{errors.at(0)};
  EXPECT_TRUE(first_error.defines("instancePosition"));

  const auto &position{first_error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}
