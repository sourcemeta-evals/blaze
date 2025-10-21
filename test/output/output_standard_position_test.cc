#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_position, annotations_with_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_string{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream schema_stream{schema_string};
  const auto schema{sourcemeta::core::parse_json(schema_stream)};

  std::istringstream instance_stream{instance_string};
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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

  const auto &annotations{result.at("annotations")};
  EXPECT_TRUE(annotations.is_array());
  EXPECT_EQ(annotations.size(), 1);

  const auto &annotation{annotations.at(0)};
  EXPECT_TRUE(annotation.defines("keywordLocation"));
  EXPECT_TRUE(annotation.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(annotation.defines("instanceLocation"));
  EXPECT_TRUE(annotation.defines("annotation"));
  EXPECT_TRUE(annotation.defines("instancePosition"));

  const auto &position{annotation.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, errors_with_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" }
    }
  })JSON"};

  const auto instance_string{R"JSON({
  "foo": 1
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream schema_stream{schema_string};
  const auto schema{sourcemeta::core::parse_json(schema_stream)};

  std::istringstream instance_stream{instance_string};
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_EQ(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.defines("keywordLocation"));
  EXPECT_TRUE(error.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(error.defines("instanceLocation"));
  EXPECT_TRUE(error.defines("error"));
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, multiline_instance) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "foo": { "type": "string" },
      "bar": { "type": "number" }
    }
  })JSON"};

  const auto instance_string{R"JSON({
  "foo": "test",
  "bar": "invalid"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream schema_stream{schema_string};
  const auto schema{sourcemeta::core::parse_json(schema_stream)};

  std::istringstream instance_stream{instance_string};
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_GE(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);

  EXPECT_TRUE(position.at(0).is_integer());
  EXPECT_TRUE(position.at(1).is_integer());
  EXPECT_TRUE(position.at(2).is_integer());
  EXPECT_TRUE(position.at(3).is_integer());

  EXPECT_GE(position.at(0).to_integer(), 1);
  EXPECT_GE(position.at(2).to_integer(), 1);
}

TEST(Output_standard_position, nested_properties) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "nested": {
        "type": "object",
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

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream schema_stream{schema_string};
  const auto schema{sourcemeta::core::parse_json(schema_stream)};

  std::istringstream instance_stream{instance_string};
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_GE(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, array_items) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "array",
    "items": { "type": "string" }
  })JSON"};

  const auto instance_string{R"JSON([
  "valid",
  123,
  "also valid"
])JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream schema_stream{schema_string};
  const auto schema{sourcemeta::core::parse_json(schema_stream)};

  std::istringstream instance_stream{instance_string};
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_GE(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

TEST(Output_standard_position, flag_format_no_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON"};

  const auto instance_string{R"JSON("test")JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream schema_stream{schema_string};
  const auto schema{sourcemeta::core::parse_json(schema_stream)};

  std::istringstream instance_stream{instance_string};
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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
  EXPECT_FALSE(result.defines("annotations"));
  EXPECT_FALSE(result.defines("errors"));
}

TEST(Output_standard_position, root_instance_position) {
  const auto schema_string{R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object"
  })JSON"};

  const auto instance_string{R"JSON("not an object")JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream schema_stream{schema_string};
  const auto schema{sourcemeta::core::parse_json(schema_stream)};

  std::istringstream instance_stream{instance_string};
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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

  const auto &errors{result.at("errors")};
  EXPECT_TRUE(errors.is_array());
  EXPECT_GE(errors.size(), 1);

  const auto &error{errors.at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}
