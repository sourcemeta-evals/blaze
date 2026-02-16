#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <sstream>
#include <tuple>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonpointer.h>

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

  const std::string input{R"JSON({
  "foo": 1
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error{result.at("errors").at(0)};
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);
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

  const std::string input{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_EQ(result.at("annotations").size(), 1);

  const auto &annotation{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation.defines("instancePosition"));
  EXPECT_TRUE(annotation.at("instancePosition").is_array());
  EXPECT_EQ(annotation.at("instancePosition").size(), 4);
}

TEST(Output_standard_basic_position, flag_ignores_positions) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string input{"\"hello\""};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
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

  const std::string input{R"JSON({
  "foo": 1
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto &error{result.at("errors").at(0)};
  const auto &pos{error.at("instancePosition")};
  EXPECT_TRUE(pos.at(0).is_integer());
  EXPECT_TRUE(pos.at(1).is_integer());
  EXPECT_TRUE(pos.at(2).is_integer());
  EXPECT_TRUE(pos.at(3).is_integer());

  const auto tracker_pos{tracker.get(sourcemeta::core::Pointer{"foo"})};
  EXPECT_TRUE(tracker_pos.has_value());
  EXPECT_EQ(pos.at(0).to_integer(),
            static_cast<std::int64_t>(std::get<0>(tracker_pos.value())));
  EXPECT_EQ(pos.at(1).to_integer(),
            static_cast<std::int64_t>(std::get<1>(tracker_pos.value())));
  EXPECT_EQ(pos.at(2).to_integer(),
            static_cast<std::int64_t>(std::get<2>(tracker_pos.value())));
  EXPECT_EQ(pos.at(3).to_integer(),
            static_cast<std::int64_t>(std::get<3>(tracker_pos.value())));
}

TEST(Output_standard_basic_position, success_no_annotations) {
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

  const std::string input{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}
