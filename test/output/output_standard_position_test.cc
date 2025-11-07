#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_position, basic_error_with_position) {
  const auto schema_text{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_text{R"JSON({
    "foo": 1
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto schema{sourcemeta::core::parse_json(schema_text)};
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

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
  EXPECT_TRUE(error.defines("instanceLocation"));
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);
}

TEST(Output_standard_position, basic_annotation_with_position) {
  const auto schema_text{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_text{R"JSON({
    "foo": "bar"
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto schema{sourcemeta::core::parse_json(schema_text)};
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_GE(result.at("annotations").size(), 1);

  // Check that at least one annotation has instancePosition
  bool found_position = false;
  for (const auto &annotation : result.at("annotations").as_array()) {
    if (annotation.defines("instancePosition")) {
      found_position = true;
      EXPECT_TRUE(annotation.at("instancePosition").is_array());
      EXPECT_EQ(annotation.at("instancePosition").size(), 4);
      break;
    }
  }
  EXPECT_TRUE(found_position);
}

TEST(Output_standard_position, flag_format_ignores_tracker) {
  const auto schema_text{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON"};

  const auto instance_text{R"JSON("hello")JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto schema{sourcemeta::core::parse_json(schema_text)};
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_FALSE(result.defines("errors"));
  EXPECT_FALSE(result.defines("annotations"));
}

TEST(Output_standard_position, nested_property_error_with_position) {
  const auto schema_text{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "user": {
        "properties": {
          "age": { "type": "number" }
        }
      }
    }
  })JSON"};

  const auto instance_text{R"JSON({
    "user": {
      "age": "not a number"
    }
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto schema{sourcemeta::core::parse_json(schema_text)};
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_GE(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);
}

TEST(Output_standard_position, array_item_error_with_position) {
  const auto schema_text{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "items": { "type": "string" }
  })JSON"};

  const auto instance_text{R"JSON([
    "valid",
    123,
    "also valid"
  ])JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto schema{sourcemeta::core::parse_json(schema_text)};
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_GE(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);
}

TEST(Output_standard_position, position_values_are_valid) {
  const auto schema_text{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_text{R"JSON({
    "foo": 123
  })JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto schema{sourcemeta::core::parse_json(schema_text)};
  const auto instance{
      sourcemeta::core::parse_json(instance_text, std::ref(tracker))};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("errors"));
  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_EQ(position.size(), 4);

  // All position values should be integers
  EXPECT_TRUE(position.at(0).is_integer());
  EXPECT_TRUE(position.at(1).is_integer());
  EXPECT_TRUE(position.at(2).is_integer());
  EXPECT_TRUE(position.at(3).is_integer());

  // Line and column numbers should be positive
  EXPECT_GT(position.at(0).to_integer(), 0);
  EXPECT_GT(position.at(1).to_integer(), 0);
  EXPECT_GT(position.at(2).to_integer(), 0);
  EXPECT_GT(position.at(3).to_integer(), 0);
}
