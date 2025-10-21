#include <gtest/gtest.h>

#include <functional>
#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

TEST(Output_standard_position, error_with_position) {
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

  const std::string instance_string{R"JSON({
  "foo": 123
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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
  EXPECT_TRUE(error.defines("instanceLocation"));
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);

  EXPECT_TRUE(error.at("instancePosition").at(0).is_integer());
  EXPECT_TRUE(error.at("instancePosition").at(1).is_integer());
  EXPECT_TRUE(error.at("instancePosition").at(2).is_integer());
  EXPECT_TRUE(error.at("instancePosition").at(3).is_integer());

  EXPECT_EQ(error.at("instancePosition").at(0).to_integer(), 2);
  EXPECT_EQ(error.at("instancePosition").at(1).to_integer(), 3);
  EXPECT_EQ(error.at("instancePosition").at(2).to_integer(), 2);
  EXPECT_EQ(error.at("instancePosition").at(3).to_integer(), 12);
}

TEST(Output_standard_position, annotation_with_position) {
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

  const std::string instance_string{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_GE(result.at("annotations").size(), 1);

  const auto &annotation{result.at("annotations").at(0)};
  EXPECT_TRUE(annotation.defines("instanceLocation"));
  EXPECT_TRUE(annotation.defines("instancePosition"));
  EXPECT_TRUE(annotation.at("instancePosition").is_array());
  EXPECT_EQ(annotation.at("instancePosition").size(), 4);

  EXPECT_TRUE(annotation.at("instancePosition").at(0).is_integer());
  EXPECT_TRUE(annotation.at("instancePosition").at(1).is_integer());
  EXPECT_TRUE(annotation.at("instancePosition").at(2).is_integer());
  EXPECT_TRUE(annotation.at("instancePosition").at(3).is_integer());
}

TEST(Output_standard_position, multiline_error_with_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "properties": {
      "nested": {
        "properties": {
          "value": { "type": "string" }
        }
      }
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string instance_string{R"JSON({
  "nested": {
    "value": 456
  }
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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
  EXPECT_TRUE(error.defines("instanceLocation"));
  const auto instance_location_pointer{
      sourcemeta::core::to_pointer(error.at("instanceLocation"))};
  EXPECT_EQ(sourcemeta::core::to_string(instance_location_pointer),
            "/nested/value");
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);

  EXPECT_EQ(error.at("instancePosition").at(0).to_integer(), 3);
  EXPECT_EQ(error.at("instancePosition").at(1).to_integer(), 5);
  EXPECT_EQ(error.at("instancePosition").at(2).to_integer(), 3);
  EXPECT_EQ(error.at("instancePosition").at(3).to_integer(), 16);
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

  const std::string instance_string{R"JSON(123)JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_FALSE(result.defines("errors"));
  EXPECT_FALSE(result.defines("annotations"));
}

TEST(Output_standard_position, root_instance_position) {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation)};

  const std::string instance_string{R"JSON(123)JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{instance_string};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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
  EXPECT_TRUE(error.defines("instanceLocation"));
  const auto instance_location_pointer{
      sourcemeta::core::to_pointer(error.at("instanceLocation"))};
  EXPECT_EQ(sourcemeta::core::to_string(instance_location_pointer), "");
  EXPECT_TRUE(error.defines("instancePosition"));
  EXPECT_TRUE(error.at("instancePosition").is_array());
  EXPECT_EQ(error.at("instancePosition").size(), 4);

  EXPECT_EQ(error.at("instancePosition").at(0).to_integer(), 1);
  EXPECT_EQ(error.at("instancePosition").at(1).to_integer(), 1);
  EXPECT_EQ(error.at("instancePosition").at(2).to_integer(), 1);
  EXPECT_EQ(error.at("instancePosition").at(3).to_integer(), 3);
}
