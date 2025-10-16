#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonpointer_position.h>

TEST(Output_standard_basic_positions, success_with_annotations) {
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

  const std::string instance_str{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  std::istringstream stream{instance_str};
  const auto instance{
      sourcemeta::core::parse_json(stream, std::ref(positions))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));

  const auto &annotations{result.at("annotations")};
  EXPECT_TRUE(annotations.is_array());
  EXPECT_GT(annotations.size(), 0);

  // Check that annotations have the standard fields
  const auto &first_annotation{annotations.at(0)};
  EXPECT_TRUE(first_annotation.defines("keywordLocation"));
  EXPECT_TRUE(first_annotation.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(first_annotation.defines("instanceLocation"));
  EXPECT_TRUE(first_annotation.defines("annotation"));

  // Check for instancePosition in the root instance annotation
  if (first_annotation.at("instanceLocation").to_string() == "") {
    EXPECT_TRUE(first_annotation.defines("instancePosition"));
    const auto &position{first_annotation.at("instancePosition")};
    EXPECT_TRUE(position.is_array());
    EXPECT_EQ(position.size(), 4);
  }
}

TEST(Output_standard_basic_positions, failure_with_errors) {
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

  const std::string instance_str{R"JSON({
  "foo": 123
})JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  std::istringstream stream{instance_str};
  const auto instance{
      sourcemeta::core::parse_json(stream, std::ref(positions))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_GT(errors.size(), 0);

  // Check that errors have the standard fields
  const auto &first_error{errors.at(0)};
  EXPECT_TRUE(first_error.defines("keywordLocation"));
  EXPECT_TRUE(first_error.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(first_error.defines("instanceLocation"));
  EXPECT_TRUE(first_error.defines("error"));

  // Check for instancePosition
  EXPECT_TRUE(first_error.defines("instancePosition"));
  const auto &position{first_error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);

  // The position should contain line and column numbers (all positive)
  EXPECT_GT(position.at(0).to_integer(), 0); // lineStart
  EXPECT_GT(position.at(1).to_integer(), 0); // columnStart
  EXPECT_GT(position.at(2).to_integer(), 0); // lineEnd
  EXPECT_GT(position.at(3).to_integer(), 0); // columnEnd
}

TEST(Output_standard_basic_positions, nested_error_with_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "nested": {
        "type": "object",
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

  const std::string instance_str{R"JSON({
  "nested": {
    "value": "not a number"
  }
})JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  std::istringstream stream{instance_str};
  const auto instance{
      sourcemeta::core::parse_json(stream, std::ref(positions))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors")};
  EXPECT_GT(errors.size(), 0);

  // Find the error for the nested value
  bool found_nested_error = false;
  for (std::size_t i = 0; i < errors.size(); i++) {
    const auto &error{errors.at(i)};
    const auto &instance_loc{error.at("instanceLocation").to_string()};
    if (instance_loc.find("value") != std::string::npos) {
      found_nested_error = true;
      EXPECT_TRUE(error.defines("instancePosition"));
      const auto &position{error.at("instancePosition")};
      EXPECT_EQ(position.size(), 4);
      break;
    }
  }

  EXPECT_TRUE(found_nested_error);
}

TEST(Output_standard_basic_positions, flag_format_ignores_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string instance_str{R"JSON("hello")JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  std::istringstream stream{instance_str};
  const auto instance{
      sourcemeta::core::parse_json(stream, std::ref(positions))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, positions)};

  // Flag format should only have "valid"
  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_FALSE(result.defines("errors"));
  EXPECT_FALSE(result.defines("annotations"));
}

TEST(Output_standard_basic_positions, array_error_with_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "items": { "type": "string" }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string instance_str{R"JSON([
  "valid",
  123,
  "also valid"
])JSON"};

  sourcemeta::core::PointerPositionTracker positions;
  std::istringstream stream{instance_str};
  const auto instance{
      sourcemeta::core::parse_json(stream, std::ref(positions))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, positions)};

  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors")};
  EXPECT_GT(errors.size(), 0);

  // Find the error for array index 1
  bool found_array_error = false;
  for (std::size_t i = 0; i < errors.size(); i++) {
    const auto &error{errors.at(i)};
    const auto &instance_loc{error.at("instanceLocation").to_string()};
    if (instance_loc.find("1") != std::string::npos) {
      found_array_error = true;
      EXPECT_TRUE(error.defines("instancePosition"));
      const auto &position{error.at("instancePosition")};
      EXPECT_EQ(position.size(), 4);
      // Line should be 3 (line 1 is '[', line 2 is '"valid"', line 3 is '123')
      EXPECT_EQ(position.at(0).to_integer(), 3);
      break;
    }
  }

  EXPECT_TRUE(found_array_error);
}
