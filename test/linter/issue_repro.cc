#include <gtest/gtest.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/default_compiler.h>
#include <sourcemeta/blaze/linter.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/jsonschema_transform.h>
#include <sourcemeta/core/jsonschema_walker.h>

TEST(IssueRepro, ValidExamplesSiblingRefDraft7) {
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
      sourcemeta::core::schema_official_resolver, [](const auto &) {});

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

TEST(IssueRepro, ValidDefaultSiblingRefDraft7) {
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
      sourcemeta::core::schema_official_resolver, [](const auto &) {});

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
