#include <gtest/gtest.h>

#include <sstream>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

TEST(Output_standard_position, annotations_with_position) {
  const auto input{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_FALSE(result.at("annotations").empty());

  const auto &first_annotation{result.at("annotations").at(0)};
  EXPECT_TRUE(first_annotation.defines("keywordLocation"));
  EXPECT_TRUE(first_annotation.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(first_annotation.defines("instanceLocation"));
  EXPECT_TRUE(first_annotation.defines("annotation"));
  EXPECT_TRUE(first_annotation.defines("instancePosition"));
  EXPECT_TRUE(first_annotation.at("instancePosition").is_array());
  EXPECT_EQ(first_annotation.at("instancePosition").size(), 4);
}

TEST(Output_standard_position, errors_with_position) {
  const auto input{R"JSON({
  "foo": 1
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_FALSE(result.at("errors").empty());

  const auto &first_error{result.at("errors").at(0)};
  EXPECT_TRUE(first_error.defines("keywordLocation"));
  EXPECT_TRUE(first_error.defines("absoluteKeywordLocation"));
  EXPECT_TRUE(first_error.defines("instanceLocation"));
  EXPECT_TRUE(first_error.defines("error"));
  EXPECT_TRUE(first_error.defines("instancePosition"));
  EXPECT_TRUE(first_error.at("instancePosition").is_array());
  EXPECT_EQ(first_error.at("instancePosition").size(), 4);
}

TEST(Output_standard_position, prettify_errors_with_position) {
  const auto input{R"JSON({
  "foo": 1
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": false,
    "errors": [
      {
        "keywordLocation": "/properties/foo/type",
        "absoluteKeywordLocation": "#/properties/foo/type",
        "instanceLocation": "/foo",
        "instancePosition": [ 2, 3, 2, 10 ],
        "error": "The value was expected to be of type string but it was of type integer"
      }
    ]
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_position, flag_format) {
  const auto input{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Flag, tracker)};

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "valid": true
  })JSON")};

  EXPECT_EQ(result, expected);
}

TEST(Output_standard_position, no_position_for_root) {
  const auto input{R"JSON({
  "foo": "bar"
})JSON"};

  sourcemeta::core::PointerPositionTracker tracker;
  std::istringstream stream{input};
  const auto instance{sourcemeta::core::parse_json(stream, std::ref(tracker))};

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

  sourcemeta::blaze::Evaluator evaluator;
  const auto result{sourcemeta::blaze::standard(
      evaluator, schema_template, instance,
      sourcemeta::blaze::StandardOutput::Basic, tracker)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));

  const auto &first_annotation{result.at("annotations").at(0)};
  EXPECT_EQ(first_annotation.at("instanceLocation"),
            sourcemeta::core::parse_json(R"JSON("")JSON"));
  EXPECT_TRUE(first_annotation.defines("instancePosition"));
}
