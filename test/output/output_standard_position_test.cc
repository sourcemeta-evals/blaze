#include <gtest/gtest.h>

#include <functional>
#include <sstream>
#include <tuple>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_position, flag_no_positions) {
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
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_position, basic_annotations) {
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

  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));

  for (const auto &unit : result.at("annotations").as_array()) {
    EXPECT_TRUE(unit.defines("instancePosition"));
    const auto &pos{unit.at("instancePosition")};
    EXPECT_TRUE(pos.is_array());
    EXPECT_EQ(pos.size(), 4);
    EXPECT_TRUE(pos.at(0).is_integer());
    EXPECT_TRUE(pos.at(1).is_integer());
    EXPECT_TRUE(pos.at(2).is_integer());
    EXPECT_TRUE(pos.at(3).is_integer());
  }
}

TEST(Output_standard_position, basic_errors) {
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

  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));

  for (const auto &unit : result.at("errors").as_array()) {
    EXPECT_TRUE(unit.defines("instancePosition"));
    const auto &pos{unit.at("instancePosition")};
    EXPECT_TRUE(pos.is_array());
    EXPECT_EQ(pos.size(), 4);
    EXPECT_TRUE(pos.at(0).is_integer());
    EXPECT_TRUE(pos.at(1).is_integer());
    EXPECT_TRUE(pos.at(2).is_integer());
    EXPECT_TRUE(pos.at(3).is_integer());
  }
}

TEST(Output_standard_position, basic_errors_match_tracker) {
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

  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error_unit{result.at("errors").at(0)};
  EXPECT_TRUE(error_unit.defines("instancePosition"));

  const auto instance_location{sourcemeta::core::to_pointer(
      error_unit.at("instanceLocation").to_string())};
  const auto expected_position{tracker.get(instance_location)};
  ASSERT_TRUE(expected_position.has_value());

  const auto &pos{error_unit.at("instancePosition")};
  EXPECT_EQ(pos.at(0).to_integer(),
            static_cast<std::int64_t>(std::get<0>(expected_position.value())));
  EXPECT_EQ(pos.at(1).to_integer(),
            static_cast<std::int64_t>(std::get<1>(expected_position.value())));
  EXPECT_EQ(pos.at(2).to_integer(),
            static_cast<std::int64_t>(std::get<2>(expected_position.value())));
  EXPECT_EQ(pos.at(3).to_integer(),
            static_cast<std::int64_t>(std::get<3>(expected_position.value())));
}

TEST(Output_standard_position, basic_annotations_match_tracker) {
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

  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_EQ(result.at("annotations").size(), 1);

  const auto &annotation_unit{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation_unit.defines("instancePosition"));

  const auto instance_location{sourcemeta::core::to_pointer(
      annotation_unit.at("instanceLocation").to_string())};
  const auto expected_position{tracker.get(instance_location)};
  ASSERT_TRUE(expected_position.has_value());

  const auto &pos{annotation_unit.at("instancePosition")};
  EXPECT_EQ(pos.at(0).to_integer(),
            static_cast<std::int64_t>(std::get<0>(expected_position.value())));
  EXPECT_EQ(pos.at(1).to_integer(),
            static_cast<std::int64_t>(std::get<1>(expected_position.value())));
  EXPECT_EQ(pos.at(2).to_integer(),
            static_cast<std::int64_t>(std::get<2>(expected_position.value())));
  EXPECT_EQ(pos.at(3).to_integer(),
            static_cast<std::int64_t>(std::get<3>(expected_position.value())));
}
