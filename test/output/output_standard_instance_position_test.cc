#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_instance_position, error_with_position) {
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

  std::istringstream instance_stream{R"JSON({
  "foo": 1
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

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

  const auto foo_position{tracker.get(sourcemeta::core::Pointer{"foo"})};
  ASSERT_TRUE(foo_position.has_value());
  EXPECT_EQ(error.at("instancePosition").at(0).to_integer(),
            std::get<0>(foo_position.value()));
  EXPECT_EQ(error.at("instancePosition").at(1).to_integer(),
            std::get<1>(foo_position.value()));
  EXPECT_EQ(error.at("instancePosition").at(2).to_integer(),
            std::get<2>(foo_position.value()));
  EXPECT_EQ(error.at("instancePosition").at(3).to_integer(),
            std::get<3>(foo_position.value()));
}

TEST(Output_standard_instance_position, annotation_with_position) {
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

  std::istringstream instance_stream{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_GE(result.at("annotations").size(), 1);

  const auto &annotation{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation.defines("instancePosition"));
  EXPECT_TRUE(annotation.at("instancePosition").is_array());
  EXPECT_EQ(annotation.at("instancePosition").size(), 4);

  const auto root_position{tracker.get(sourcemeta::core::Pointer{})};
  ASSERT_TRUE(root_position.has_value());
  EXPECT_EQ(annotation.at("instancePosition").at(0).to_integer(),
            std::get<0>(root_position.value()));
  EXPECT_EQ(annotation.at("instancePosition").at(1).to_integer(),
            std::get<1>(root_position.value()));
  EXPECT_EQ(annotation.at("instancePosition").at(2).to_integer(),
            std::get<2>(root_position.value()));
  EXPECT_EQ(annotation.at("instancePosition").at(3).to_integer(),
            std::get<3>(root_position.value()));
}

TEST(Output_standard_instance_position, flag_format_no_position) {
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

  std::istringstream instance_stream{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance{
      sourcemeta::core::parse_json(instance_stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Flag)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
  EXPECT_FALSE(result.defines("errors"));
  EXPECT_FALSE(result.defines("annotations"));
}

TEST(Output_standard_instance_position, missing_position_omitted) {
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

  sourcemeta::core::PointerPositionTracker tracker;

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
  EXPECT_FALSE(error.defines("instancePosition"));
}
