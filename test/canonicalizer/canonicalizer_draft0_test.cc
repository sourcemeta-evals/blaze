#include <sourcemeta/core/test.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <filesystem> // std::filesystem::path
#include <memory>     // std::unique_ptr

#include "canonicalizer_test_utils.h"

namespace {
auto compiled_metaschema() -> const sourcemeta::blaze::Template & {
  static const sourcemeta::blaze::Template SCHEMA_TEMPLATE{
      sourcemeta::blaze::compile(
          sourcemeta::core::read_json(std::filesystem::path{SCHEMAS_PATH} /
                                      "canonical-draft1.json"),
          sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver,
          sourcemeta::blaze::default_schema_compiler)};
  return SCHEMA_TEMPLATE;
}
} // namespace

TEST(type_boolean_as_enum) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": "boolean"
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "enum": [ false, true ]
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(integer_minimal) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": "integer"
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": "integer",
    "maxDecimal": 0
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(array_minimal) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": "array"
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": "array",
    "items": {},
    "minItems": 0
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(object_with_property_optional_implicit) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": "object",
    "properties": {
      "name": { "type": "string" }
    }
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": "object",
    "properties": {
      "name": { "type": "string", "minLength": 0, "optional": false }
    },
    "additionalProperties": {}
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(type_union_schema_variant) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": [ "array", { "type": "object" } ]
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "type": [
      { "type": "array", "minItems": 0, "items": {} },
      { "type": "object", "properties": {}, "additionalProperties": {} }
    ]
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(unsatisfiable_empty_enum) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "enum": []
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "disallow": [ "any" ]
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(top_level_ref_with_siblings) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/schema#",
    "$ref": "#",
    "title": "Standing beside the reference"
  })JSON");

  const auto original = document;

  try {
    sourcemeta::blaze::canonicalize(document, sourcemeta::core::schema_walker,
                                    canonicalizer_test_resolver);
    FAIL();
  } catch (const sourcemeta::core::SchemaError &error) {
    EXPECT_STREQ(error.what(),
                 "Cannot canonicalise a JSON Schema Draft 7 or older with a "
                 "top-level `$ref` (which overrides sibling keywords) without "
                 "introducing undefined behavior");
    EXPECT_EQ(document, original);
  } catch (...) {
    FAIL();
  }
}

TEST(top_level_ref_with_siblings_in_the_hyper_schema) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-00/hyper-schema#",
    "$ref": "#",
    "title": "Standing beside the reference"
  })JSON");

  const auto original = document;

  try {
    sourcemeta::blaze::canonicalize(document, sourcemeta::core::schema_walker,
                                    canonicalizer_test_resolver);
    FAIL();
  } catch (const sourcemeta::core::SchemaError &error) {
    EXPECT_STREQ(error.what(),
                 "Cannot canonicalise a JSON Schema Draft 7 or older with a "
                 "top-level `$ref` (which overrides sibling keywords) without "
                 "introducing undefined behavior");
    EXPECT_EQ(document, original);
  } catch (...) {
    FAIL();
  }
}
