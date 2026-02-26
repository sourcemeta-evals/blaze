#include <gtest/gtest.h>

#include <functional> // std::ref
#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_position, flag_success) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string input{R"JSON("hello")JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(input, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

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

  const std::string input{R"JSON(1)JSON"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(input, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

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

  const std::string input{"{\n  \"foo\": 1\n}"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(input, std::ref(tracker))};

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

TEST(Output_standard_position, basic_error_with_position_prettify) {
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

  const std::string input{"{\n  \"foo\": 1\n}"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(input, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  const auto &error_unit{result.at("errors").at(0)};
  EXPECT_TRUE(error_unit.defines("instancePosition"));

  const auto &pos{error_unit.at("instancePosition")};

  // For input "{\n  \"foo\": 1\n}", the pointer /foo maps to the
  // value 1 whose position is tracked by the PointerPositionTracker
  const auto position{tracker.get(sourcemeta::core::Pointer{"foo"})};
  ASSERT_TRUE(position.has_value());
  EXPECT_EQ(pos.at(0).to_integer(),
            static_cast<std::int64_t>(std::get<0>(position.value())));
  EXPECT_EQ(pos.at(1).to_integer(),
            static_cast<std::int64_t>(std::get<1>(position.value())));
  EXPECT_EQ(pos.at(2).to_integer(),
            static_cast<std::int64_t>(std::get<2>(position.value())));
  EXPECT_EQ(pos.at(3).to_integer(),
            static_cast<std::int64_t>(std::get<3>(position.value())));
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

  const std::string input{"{\n  \"foo\": \"bar\"\n}"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(input, std::ref(tracker))};

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

  // The annotation is on the root "", which corresponds to the
  // entire document position
  const auto &pos{annotation_unit.at("instancePosition")};
  const auto position{tracker.get(sourcemeta::core::Pointer{})};
  ASSERT_TRUE(position.has_value());
  EXPECT_EQ(pos.at(0).to_integer(),
            static_cast<std::int64_t>(std::get<0>(position.value())));
  EXPECT_EQ(pos.at(1).to_integer(),
            static_cast<std::int64_t>(std::get<1>(position.value())));
  EXPECT_EQ(pos.at(2).to_integer(),
            static_cast<std::int64_t>(std::get<2>(position.value())));
  EXPECT_EQ(pos.at(3).to_integer(),
            static_cast<std::int64_t>(std::get<3>(position.value())));
}

TEST(Output_standard_position, basic_success_no_annotations) {
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

  const std::string input{"{\n  \"foo\": \"bar\"\n}"};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{sourcemeta::core::parse_json(input, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}
