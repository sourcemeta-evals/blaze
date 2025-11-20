#include <gtest/gtest.h>

#include <sourcemeta/blaze/linter.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

static auto transformer_callback_error(const sourcemeta::core::Pointer &,
                                       const std::string_view,
                                       const std::string_view,
                                       const std::string_view) -> void {
  throw std::runtime_error("The transform callback must not be called");
}

TEST(Linter, ValidExamples_RefSibling_Draft7) {
  sourcemeta::core::SchemaTransformer bundle;
  bundle.add<sourcemeta::blaze::ValidExamples>(
      sourcemeta::blaze::default_schema_compiler);

  auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-07/schema#",
    "properties": {
      "foo": { "$ref": "#/definitions/helper", "examples": [ 1 ] }
    },
    "definitions": {
      "helper": { "type": "string" }
    }
  })JSON")};

  const auto result = bundle.apply(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver, transformer_callback_error);

  EXPECT_TRUE(result);

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-07/schema#",
    "properties": {
      "foo": { "$ref": "#/definitions/helper", "examples": [ 1 ] }
    },
    "definitions": {
      "helper": { "type": "string" }
    }
  })JSON")};

  EXPECT_EQ(schema, expected);
}

TEST(Linter, ValidDefault_RefSibling_Draft7) {
  sourcemeta::core::SchemaTransformer bundle;
  bundle.add<sourcemeta::blaze::ValidDefault>(
      sourcemeta::blaze::default_schema_compiler);

  auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-07/schema#",
    "properties": {
      "foo": { "$ref": "#/definitions/helper", "default": 1 }
    },
    "definitions": {
      "helper": { "type": "string" }
    }
  })JSON")};

  const auto result = bundle.apply(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver, transformer_callback_error);

  EXPECT_TRUE(result);

  const auto expected{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-07/schema#",
    "properties": {
      "foo": { "$ref": "#/definitions/helper", "default": 1 }
    },
    "definitions": {
      "helper": { "type": "string" }
    }
  })JSON")};

  EXPECT_EQ(schema, expected);
}
