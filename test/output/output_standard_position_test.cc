#include <gtest/gtest.h>

#include <functional>
#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_position, basic_success_with_positions) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};
  const auto schema{sourcemeta::core::parse_json(schema_string)};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const auto instance_string{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());

  const auto &annotations{result.at("annotations")};
  EXPECT_TRUE(annotations.is_array());
  EXPECT_EQ(annotations.size(), 1);

  const auto &annotation{annotations.at(0)};
  EXPECT_TRUE(annotation.is_object());
  EXPECT_TRUE(annotation.defines("keywordLocation"));
  EXPECT_TRUE(annotation.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(annotation.defines("instanceLocation"));
  EXPECT_TRUE(annotation.defines("annotation"));
}

TEST(Output_standard_position, basic_error_with_positions) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};
  const auto schema{sourcemeta::core::parse_json(schema_string)};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance_string{R"JSON({
  "foo": 123
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_EQ(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.is_object());
  EXPECT_TRUE(error.defines("keywordLocation"));
  EXPECT_TRUE(error.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(error.defines("instanceLocation"));
  EXPECT_TRUE(error.defines("error"));
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &instance_position{error.at("instancePosition")};
  EXPECT_TRUE(instance_position.is_array());
  EXPECT_EQ(instance_position.size(), 4);
}

TEST(Output_standard_position, nested_property_with_position) {
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
  const auto schema{sourcemeta::core::parse_json(schema_string)};

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
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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
  EXPECT_EQ(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.is_object());
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &instance_position{error.at("instancePosition")};
  EXPECT_TRUE(instance_position.is_array());
  EXPECT_EQ(instance_position.size(), 4);
}

TEST(Output_standard_position, flag_format_no_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON"};
  const auto schema{sourcemeta::core::parse_json(schema_string)};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance_string{R"JSON("test")JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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

TEST(Output_standard_position, array_error_with_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "items": { "type": "string" }
  })JSON"};
  const auto schema{sourcemeta::core::parse_json(schema_string)};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto instance_string{R"JSON([
  "valid",
  123,
  "also valid"
])JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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
  EXPECT_GE(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.is_object());
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &instance_position{error.at("instancePosition")};
  EXPECT_TRUE(instance_position.is_array());
  EXPECT_EQ(instance_position.size(), 4);
}
