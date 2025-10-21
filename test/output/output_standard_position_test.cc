#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_position, basic_error_with_position) {
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

TEST(Output_standard_position, basic_annotation_with_position) {
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
  EXPECT_GT(result.at("annotations").size(), 0);

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

TEST(Output_standard_position, flag_format_no_position) {
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
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_string, std::ref(tracker))};

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

TEST(Output_standard_position, nested_property_position) {
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
    "value": "wrong"
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
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));

  const auto &position{error.at("instancePosition")};
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
  EXPECT_TRUE(position.at(0).is_integer());
  EXPECT_TRUE(position.at(1).is_integer());
  EXPECT_TRUE(position.at(2).is_integer());
  EXPECT_TRUE(position.at(3).is_integer());
}

TEST(Output_standard_position, array_item_position) {
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
  EXPECT_GE(result.at("errors").size(), 1);

  bool found_error_at_index_1 = false;
  for (const auto &error : result.at("errors").as_array()) {
    if (error.defines("instanceLocation") &&
        error.at("instanceLocation").to_string() == "/1") {
      found_error_at_index_1 = true;
      EXPECT_TRUE(error.defines("instancePosition"));
      const auto &position{error.at("instancePosition")};
      EXPECT_TRUE(position.is_array());
      EXPECT_EQ(position.size(), 4);
      break;
    }
  }
  EXPECT_TRUE(found_error_at_index_1);
}
