#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/core/jsonpointer_position.h>

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

  const std::string json_input = R"JSON({
    "foo": 1
  })JSON";

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{json_input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  // We expect instancePosition to be present
  ASSERT_TRUE(result.defines("errors"));
  const auto &errors = result.at("errors");
  ASSERT_TRUE(errors.is_array());
  ASSERT_FALSE(errors.empty());

  const auto &error = errors.at(0);
  ASSERT_TRUE(error.defines("instancePosition"));
  const auto &position = error.at("instancePosition");
  ASSERT_TRUE(position.is_array());
  ASSERT_EQ(position.size(), 4);

  // "foo": 1
  // The value 1 is at line 2.
  // "foo": 1
  //        ^ column 12 (approximate, need to verify exact position)

  // Let's just check types for now
  EXPECT_TRUE(position.at(0).is_integer()); // lineStart
  EXPECT_TRUE(position.at(1).is_integer()); // columnStart
  EXPECT_TRUE(position.at(2).is_integer()); // lineEnd
  EXPECT_TRUE(position.at(3).is_integer()); // columnEnd
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

  const std::string json_input = R"JSON({
    "foo": "bar"
  })JSON";

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{json_input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  ASSERT_TRUE(result.defines("annotations"));
  const auto &annotations = result.at("annotations");
  ASSERT_TRUE(annotations.is_array());
  ASSERT_FALSE(annotations.empty());

  const auto &annotation = annotations.at(0);
  // The annotation is on "properties" which matches the root object properties
  // Wait, the annotation is for "foo" property?
  // In the previous test `success_1_exhaustive`:
  // "keywordLocation": "/properties",
  // "annotation": [ "foo" ]
  // This annotation is emitted by `properties` keyword on the instance object.
  // So the instance location is root "".

  ASSERT_TRUE(annotation.defines("instancePosition"));
  const auto &position = annotation.at("instancePosition");
  ASSERT_TRUE(position.is_array());
  ASSERT_EQ(position.size(), 4);
}
