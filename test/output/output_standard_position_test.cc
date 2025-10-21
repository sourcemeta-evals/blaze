#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_position, basic_success_with_annotations) {
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

  const auto input{R"JSON({
  "foo": "bar"
})JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());

  const auto &annotations{result.at("annotations").as_array()};
  EXPECT_GT(annotations.size(), 0);

  // Check that the annotation has instancePosition
  const auto &first_annotation{*annotations.begin()};
  EXPECT_TRUE(first_annotation.defines("instanceLocation"));
  EXPECT_TRUE(first_annotation.defines("instancePosition"));
  EXPECT_TRUE(first_annotation.at("instancePosition").is_array());

  const auto &position{first_annotation.at("instancePosition").as_array()};
  EXPECT_EQ(position.size(), 4);
  // Verify all elements are integers
  for (const auto &elem : position) {
    EXPECT_TRUE(elem.is_integer());
  }
}

TEST(Output_standard_position, basic_failure_with_errors) {
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

  const auto input{R"JSON({
  "foo": 123
})JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());

  const auto &errors{result.at("errors").as_array()};
  EXPECT_GT(errors.size(), 0);

  // Check that the error has instancePosition
  const auto &first_error{*errors.begin()};
  EXPECT_TRUE(first_error.defines("instanceLocation"));
  EXPECT_TRUE(first_error.defines("instancePosition"));
  EXPECT_TRUE(first_error.at("instancePosition").is_array());

  const auto &position{first_error.at("instancePosition").as_array()};
  EXPECT_EQ(position.size(), 4);
  // Verify all elements are integers
  for (const auto &elem : position) {
    EXPECT_TRUE(elem.is_integer());
  }
}

TEST(Output_standard_position, flag_format_no_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto input{R"JSON("hello")JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Flag)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  // Flag format should not have annotations or errors
  EXPECT_FALSE(result.defines("annotations"));
  EXPECT_FALSE(result.defines("errors"));
}

TEST(Output_standard_position, nested_property_positions) {
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

  const auto input{R"JSON({
  "nested": {
    "value": "not a number"
  }
})JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors").as_array()};
  EXPECT_GT(errors.size(), 0);

  // Check that nested error has instancePosition
  const auto &first_error{*errors.begin()};
  EXPECT_TRUE(first_error.defines("instanceLocation"));
  EXPECT_TRUE(first_error.defines("instancePosition"));

  const auto &position{first_error.at("instancePosition").as_array()};
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, array_item_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "items": { "type": "string" }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const auto input{R"JSON([
  "valid",
  123,
  "also valid"
])JSON"};
  std::istringstream stream{input};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors").as_array()};
  EXPECT_GT(errors.size(), 0);

  // Check that array item error has instancePosition
  const auto &first_error{*errors.begin()};
  EXPECT_TRUE(first_error.defines("instanceLocation"));
  EXPECT_TRUE(first_error.defines("instancePosition"));

  const auto &position{first_error.at("instancePosition").as_array()};
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, missing_position_gracefully_handled) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  // Create instance without tracker
  const sourcemeta::core::JSON instance{123};

  // Create an empty tracker (no positions tracked)
  sourcemeta::core::PointerPositionTracker tracker;

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &errors{result.at("errors").as_array()};
  EXPECT_GT(errors.size(), 0);

  // Check that error exists but may not have instancePosition
  // (since tracker was empty)
  const auto &first_error{*errors.begin()};
  EXPECT_TRUE(first_error.defines("instanceLocation"));
  // instancePosition should not be present if position is not available
  EXPECT_FALSE(first_error.defines("instancePosition"));
}
