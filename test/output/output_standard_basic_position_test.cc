#include <gtest/gtest.h>

#include <functional>
#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_basic_position, flag_valid) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string input_string{R"JSON({
  "foo": "bar"
})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream input_stream{input_string};
  const auto instance{
      sourcemeta::core::parse_json(input_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic_position, flag_invalid) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string input_string{R"JSON({
  "foo": 1
})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream input_stream{input_string};
  const auto instance{
      sourcemeta::core::parse_json(input_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": false
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_basic_position, errors_with_position) {
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

  const std::string input_string{R"JSON({
  "foo": 1
})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream input_stream{input_string};
  const auto instance{
      sourcemeta::core::parse_json(input_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_EQ(result.at("errors").size(), 1);
  const auto &error_unit{result.at("errors").at(0)};
  EXPECT_TRUE(error_unit.defines("instancePosition"));
  EXPECT_TRUE(error_unit.at("instancePosition").is_array());
  EXPECT_EQ(error_unit.at("instancePosition").size(), 4);
}

TEST(Output_standard_basic_position, annotations_with_position) {
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

  const std::string input_string{R"JSON({
  "foo": "bar"
})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream input_stream{input_string};
  const auto instance{
      sourcemeta::core::parse_json(input_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_EQ(result.at("annotations").size(), 1);
  const auto &annotation_unit{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation_unit.defines("instancePosition"));
  EXPECT_TRUE(annotation_unit.at("instancePosition").is_array());
  EXPECT_EQ(annotation_unit.at("instancePosition").size(), 4);
}

TEST(Output_standard_basic_position, error_position_values) {
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

  const std::string input_string{R"JSON({
  "foo": 1
})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream input_stream{input_string};
  const auto instance{
      sourcemeta::core::parse_json(input_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto &error_unit{result.at("errors").at(0)};
  const auto &pos{error_unit.at("instancePosition")};

  const auto foo_position{tracker.get(sourcemeta::core::Pointer{"foo"})};
  ASSERT_TRUE(foo_position.has_value());

  EXPECT_EQ(pos.at(0).to_integer(), std::get<0>(foo_position.value()));
  EXPECT_EQ(pos.at(1).to_integer(), std::get<1>(foo_position.value()));
  EXPECT_EQ(pos.at(2).to_integer(), std::get<2>(foo_position.value()));
  EXPECT_EQ(pos.at(3).to_integer(), std::get<3>(foo_position.value()));
}

TEST(Output_standard_basic_position, annotation_position_values) {
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

  const std::string input_string{R"JSON({
  "foo": "bar"
})JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream input_stream{input_string};
  const auto instance{
      sourcemeta::core::parse_json(input_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto &annotation_unit{result.at("annotations").at(0)};
  const auto &pos{annotation_unit.at("instancePosition")};

  const auto root_position{tracker.get(sourcemeta::core::Pointer{})};
  ASSERT_TRUE(root_position.has_value());

  EXPECT_EQ(pos.at(0).to_integer(), std::get<0>(root_position.value()));
  EXPECT_EQ(pos.at(1).to_integer(), std::get<1>(root_position.value()));
  EXPECT_EQ(pos.at(2).to_integer(), std::get<2>(root_position.value()));
  EXPECT_EQ(pos.at(3).to_integer(), std::get<3>(root_position.value()));
}

TEST(Output_standard_basic_position, no_position_without_tracker) {
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

  const auto instance{sourcemeta::core::parse_json(R"JSON({
    "foo": 1
  })JSON")};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  const auto &error_unit{result.at("errors").at(0)};
  EXPECT_FALSE(error_unit.defines("instancePosition"));
}
