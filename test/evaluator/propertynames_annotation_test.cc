#include <gtest/gtest.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

TEST(PropertyNamesAnnotation, title_annotation_not_emitted) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "propertyNames": {
      "title": "Test"
    }
  })JSON")};

  const sourcemeta::core::JSON instance{sourcemeta::core::parse_json(R"JSON({
    "foo": 1
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  sourcemeta::blaze::SimpleOutput output{instance};
  sourcemeta::blaze::Evaluator evaluator;
  const auto result{evaluator.validate(schema_template, instance, std::ref(output))};

  EXPECT_TRUE(result);
  
  bool found_title_annotation = false;
  for (const auto &annotation : output.annotations()) {
    if (annotation.first.instance_location.size() == 1 &&
        annotation.first.instance_location.back().is_property() &&
        annotation.first.instance_location.back().to_property() == "foo") {
      for (const auto &value : annotation.second) {
        if (value.is_string() && value.to_string() == "Test") {
          found_title_annotation = true;
        }
      }
    }
  }
  EXPECT_FALSE(found_title_annotation) 
    << "Title annotation should not be emitted for property names";
}

TEST(PropertyNamesAnnotation, format_annotation_not_emitted) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "propertyNames": {
      "format": "email"
    }
  })JSON")};

  const sourcemeta::core::JSON instance{sourcemeta::core::parse_json(R"JSON({
    "foo": 1
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  sourcemeta::blaze::SimpleOutput output{instance};
  sourcemeta::blaze::Evaluator evaluator;
  const auto result{evaluator.validate(schema_template, instance, std::ref(output))};

  EXPECT_TRUE(result);
  
  bool found_format_annotation = false;
  for (const auto &annotation : output.annotations()) {
    if (annotation.first.instance_location.size() == 1 &&
        annotation.first.instance_location.back().is_property() &&
        annotation.first.instance_location.back().to_property() == "foo") {
      for (const auto &value : annotation.second) {
        if (value.is_string() && value.to_string() == "email") {
          found_format_annotation = true;
        }
      }
    }
  }
  EXPECT_FALSE(found_format_annotation) 
    << "Format annotation should not be emitted for property names";
}

TEST(PropertyNamesAnnotation, content_annotations_not_emitted) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "propertyNames": {
      "contentEncoding": "base64",
      "contentMediaType": "application/json"
    }
  })JSON")};

  const sourcemeta::core::JSON instance{sourcemeta::core::parse_json(R"JSON({
    "foo": 1
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  sourcemeta::blaze::SimpleOutput output{instance};
  sourcemeta::blaze::Evaluator evaluator;
  const auto result{evaluator.validate(schema_template, instance, std::ref(output))};

  EXPECT_TRUE(result);
  
  bool found_content_encoding = false;
  bool found_content_media_type = false;
  for (const auto &annotation : output.annotations()) {
    if (annotation.first.instance_location.size() == 1 &&
        annotation.first.instance_location.back().is_property() &&
        annotation.first.instance_location.back().to_property() == "foo") {
      for (const auto &value : annotation.second) {
        if (value.is_string()) {
          if (value.to_string() == "base64") {
            found_content_encoding = true;
          }
          if (value.to_string() == "application/json") {
            found_content_media_type = true;
          }
        }
      }
    }
  }
  EXPECT_FALSE(found_content_encoding) 
    << "ContentEncoding annotation should not be emitted for property names";
  EXPECT_FALSE(found_content_media_type) 
    << "ContentMediaType annotation should not be emitted for property names";
}
