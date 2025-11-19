#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_position, flag_success) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  std::istringstream stream{R"JSON({
    "foo": "bar"
  })JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Flag)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_position, flag_failure) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  std::istringstream stream{R"JSON({
    "foo": "bar"
  })JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Flag)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": false
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_position, basic_error_with_position) {
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

  std::istringstream stream{R"JSON({
  "foo": 1
})JSON"};
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
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);

  // Check that position values are integers
  EXPECT_TRUE(error.at("instancePosition").at(0).is_integer());
  EXPECT_TRUE(error.at("instancePosition").at(1).is_integer());
  EXPECT_TRUE(error.at("instancePosition").at(2).is_integer());
  EXPECT_TRUE(error.at("instancePosition").at(3).is_integer());
}

TEST(Output_standard_position, basic_annotation_with_position) {
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

  std::istringstream stream{R"JSON({
  "foo": "bar"
})JSON"};
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
  EXPECT_EQ(result.at("annotations").size(), 1);

  const auto &annotation{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation.defines("instancePosition"));
  EXPECT_TRUE(annotation.at("instancePosition").is_array());
  EXPECT_EQ(annotation.at("instancePosition").size(), 4);

  // Check that position values are integers
  EXPECT_TRUE(annotation.at("instancePosition").at(0).is_integer());
  EXPECT_TRUE(annotation.at("instancePosition").at(1).is_integer());
  EXPECT_TRUE(annotation.at("instancePosition").at(2).is_integer());
  EXPECT_TRUE(annotation.at("instancePosition").at(3).is_integer());
}

TEST(Output_standard_position, basic_error_position_values) {
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

  // Single line JSON for predictable positions
  std::istringstream stream{R"JSON({"foo": 1})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  // The error is at /foo which has value 1
  const auto &pos{error.at("instancePosition")};
  // All positions should be on line 1
  EXPECT_EQ(pos.at(0).to_integer(), 1); // lineStart
  EXPECT_EQ(pos.at(2).to_integer(), 1); // lineEnd
  // Column values should be positive
  EXPECT_GT(pos.at(1).to_integer(), 0); // columnStart
  EXPECT_GT(pos.at(3).to_integer(), 0); // columnEnd
}

TEST(Output_standard_position, basic_annotation_position_values) {
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

  // Single line JSON for predictable positions
  std::istringstream stream{R"JSON({"foo": "bar"})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_EQ(result.at("annotations").size(), 1);

  const auto &annotation{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation.defines("instancePosition"));

  // The annotation is at the root "" which is the whole object
  const auto &pos{annotation.at("instancePosition")};
  // All positions should be on line 1
  EXPECT_EQ(pos.at(0).to_integer(), 1); // lineStart
  EXPECT_EQ(pos.at(2).to_integer(), 1); // lineEnd
  // Column values should be positive
  EXPECT_GT(pos.at(1).to_integer(), 0); // columnStart
  EXPECT_GT(pos.at(3).to_integer(), 0); // columnEnd
}

TEST(Output_standard_position, basic_nested_error_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "outer": {
        "properties": {
          "inner": { "type": "string" }
        }
      }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  std::istringstream stream{R"JSON({"outer": {"inner": 42}})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);

  // The error is at /outer/inner which has value 42
  const auto &pos{error.at("instancePosition")};
  // All positions should be on line 1
  EXPECT_EQ(pos.at(0).to_integer(), 1); // lineStart
  EXPECT_EQ(pos.at(2).to_integer(), 1); // lineEnd
  // Column values should be positive
  EXPECT_GT(pos.at(1).to_integer(), 0); // columnStart
  EXPECT_GT(pos.at(3).to_integer(), 0); // columnEnd
}

TEST(Output_standard_position, basic_array_item_error_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "items": { "type": "string" }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  std::istringstream stream{R"JSON(["foo", 123, "bar"])JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("errors"));
  // At least one error for the invalid array item
  EXPECT_GE(result.at("errors").size(), 1);

  // Check that all errors have position information
  for (const auto &error : result.at("errors").as_array()) {
    EXPECT_TRUE(error.defines("instancePosition"));
    EXPECT_TRUE(error.at("instancePosition").is_array());
    EXPECT_EQ(error.at("instancePosition").size(), 4);

    const auto &pos{error.at("instancePosition")};
    // All positions should be on line 1
    EXPECT_EQ(pos.at(0).to_integer(), 1); // lineStart
    EXPECT_EQ(pos.at(2).to_integer(), 1); // lineEnd
    // Column values should be positive
    EXPECT_GT(pos.at(1).to_integer(), 0); // columnStart
    EXPECT_GT(pos.at(3).to_integer(), 0); // columnEnd
  }
}

TEST(Output_standard_position, multiline_json_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "name": { "type": "string" },
      "age": { "type": "integer" }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  std::istringstream stream{R"JSON({
  "name": 123,
  "age": "not a number"
})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("errors"));
  // At least one error should have position info
  EXPECT_GE(result.at("errors").size(), 1);

  for (const auto &error : result.at("errors").as_array()) {
    EXPECT_TRUE(error.defines("instancePosition"));
    EXPECT_TRUE(error.at("instancePosition").is_array());
    EXPECT_EQ(error.at("instancePosition").size(), 4);
  }
}
