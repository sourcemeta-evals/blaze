#include <sourcemeta/core/test.h>

#include <sourcemeta/blaze/codegen.h>
#include <sourcemeta/core/jsonschema.h>

#include <sstream> // std::ostringstream

#include "codegen_test_utils.h"

TEST(unsupported_dialect_draft3) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-03/schema#",
    "type": "string"
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    EXPECT_STREQ(error.what(), "Unsupported required vocabulary");
  }
}

TEST(unsupported_keyword_error_not) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "not": { "type": "string" }
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (const sourcemeta::blaze::CodegenUnsupportedKeywordError &error) {
    EXPECT_STREQ(error.what(), "Unsupported keyword in subschema");
  }
}

TEST(unsupported_keyword_value_error_type_not_string) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": 123
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected a string value");
  }
}

TEST(unknown_type_value_ignored) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "foo"
  })JSON")};

  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  std::ostringstream output;
  sourcemeta::blaze::generate<sourcemeta::blaze::TypeScript>(output, result);

  EXPECT_EQ(output.str(), "export type Schema_5 = number;\n"
                          "\n"
                          "export type Schema_4 = string;\n"
                          "\n"
                          "export type Schema_3Items = unknown;\n"
                          "\n"
                          "export type Schema_3 = Schema_3Items[];\n"
                          "\n"
                          "export type Schema_2 = Record<string, unknown>;\n"
                          "\n"
                          "export type Schema_1 = boolean;\n"
                          "\n"
                          "export type Schema_0 = null;\n"
                          "\n"
                          "export type Schema =\n"
                          "  Schema_0 |\n"
                          "  Schema_1 |\n"
                          "  Schema_2 |\n"
                          "  Schema_3 |\n"
                          "  Schema_4 |\n"
                          "  Schema_5;\n");
}

TEST(unsupported_keyword_value_error_properties_not_object) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "$id": "https://example.com",
    "properties": true
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected an object value");
    EXPECT_EQ(error.keyword(), "properties");
    EXPECT_AS_STRING(error.pointer(), "/anyOf/2");
  }
}

TEST(unsupported_keyword_value_error_properties_not_object_nested) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": { "foo": { "type": "object", "properties": true } }
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected an object value");
    EXPECT_EQ(error.keyword(), "properties");
    EXPECT_AS_STRING(error.pointer(), "/properties/foo");
  }
}

TEST(unsupported_keyword_value_error_required_not_array) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": { "foo": { "type": "string" } },
    "required": true
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected an array value");
    EXPECT_EQ(error.keyword(), "required");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_required_item_not_string) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": { "foo": { "type": "string" } },
    "required": [ 5 ]
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected an array of string values");
    EXPECT_EQ(error.keyword(), "required");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_pattern_properties_not_object) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "patternProperties": true
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected an object value");
    EXPECT_EQ(error.keyword(), "patternProperties");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_prefix_items_not_array) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "array",
    "prefixItems": true
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected an array value");
    EXPECT_EQ(error.keyword(), "prefixItems");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_enum_not_array) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "enum": 5
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected an array value");
    EXPECT_EQ(error.keyword(), "enum");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_any_of_not_array) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "anyOf": true
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected a non-empty array value");
    EXPECT_EQ(error.keyword(), "anyOf");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_any_of_empty) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "anyOf": []
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected a non-empty array value");
    EXPECT_EQ(error.keyword(), "anyOf");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_one_of_not_array) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "oneOf": 5
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected a non-empty array value");
    EXPECT_EQ(error.keyword(), "oneOf");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_one_of_empty) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "oneOf": []
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected a non-empty array value");
    EXPECT_EQ(error.keyword(), "oneOf");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_all_of_not_array) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "allOf": { "type": "string" }
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected a non-empty array value");
    EXPECT_EQ(error.keyword(), "allOf");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_value_error_all_of_empty) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "allOf": []
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    EXPECT_STREQ(error.what(), "Expected a non-empty array value");
    EXPECT_EQ(error.keyword(), "allOf");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unsupported_keyword_error_draft4_empty_enum) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-04/schema#",
    "enum": []
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (const sourcemeta::blaze::CodegenUnsupportedKeywordError &error) {
    EXPECT_STREQ(error.what(), "Unsupported keyword in subschema");
    EXPECT_EQ(error.keyword(), "not");
    // Draft 4 canonicalises into its graph form, so the subschema that cannot
    // be generated is an entry rather than the root
    EXPECT_AS_STRING(error.pointer(), "/definitions/0");
  }
}

TEST(draft7_empty_enum) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-07/schema#",
    "enum": []
  })JSON")};

  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  std::ostringstream output;
  sourcemeta::blaze::generate<sourcemeta::blaze::TypeScript>(output, result);

  EXPECT_EQ(output.str(), "export type Schema = never;\n");
}

TEST(unexpected_schema_error_required_property_not_allowed) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": { "foo": { "type": "string" } },
    "required": [ "bar" ],
    "additionalProperties": false
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (const sourcemeta::blaze::CodegenUnexpectedSchemaError &error) {
    EXPECT_STREQ(error.what(),
                 "This schema requires a property that it does not allow");
    EXPECT_AS_STRING(error.pointer(), "");
  }
}

TEST(unexpected_schema_error_required_property_not_allowed_nested) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
      "foo": {
        "type": "object",
        "properties": { "qux": { "type": "string" } },
        "required": [ "bar" ],
        "additionalProperties": false
      }
    }
  })JSON")};

  try {
    sourcemeta::blaze::compile(schema, sourcemeta::core::schema_walker,
                               sourcemeta::core::schema_resolver,
                               sourcemeta::blaze::default_compiler);
    FAIL();
  } catch (const sourcemeta::blaze::CodegenUnexpectedSchemaError &error) {
    EXPECT_STREQ(error.what(),
                 "This schema requires a property that it does not allow");
    EXPECT_AS_STRING(error.pointer(), "/properties/foo");
  }
}

TEST(required_property_not_in_properties_open) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": { "foo": { "type": "string" } },
    "required": [ "bar" ]
  })JSON")};

  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  std::ostringstream output;
  sourcemeta::blaze::generate<sourcemeta::blaze::TypeScript>(output, result);

  EXPECT_EQ(output.str(), "export type SchemaFoo = string;\n"
                          "\n"
                          "export type SchemaBar = unknown;\n"
                          "\n"
                          "export interface Schema {\n"
                          "  \"foo\"?: SchemaFoo;\n"
                          "  \"bar\": SchemaBar;\n"
                          "  [key: string]: unknown | undefined;\n"
                          "}\n");
}

TEST(draft4_graph_form_generates) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-04/schema#",
    "type": "string"
  })JSON")};

  // Draft 4 canonicalises into its graph form, so what reaches codegen is a
  // wrapper over one `definitions` entry rather than the schema as written.
  // Generating from it means the `definitions` container is understood and the
  // references into it are followed
  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  using namespace sourcemeta::blaze;

  EXPECT_EQ(result.size(), 3);
  EXPECT_IR_SCALAR(result, 0, String, "/definitions/0");
}

TEST(draft6_graph_form_generates) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "string"
  })JSON")};

  // Draft 6 canonicalises into its graph form too, so what reaches codegen is
  // a wrapper over one `definitions` entry rather than the schema as written
  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  using namespace sourcemeta::blaze;

  EXPECT_EQ(result.size(), 3);
  EXPECT_IR_SCALAR(result, 0, String, "/definitions/0");
}

TEST(draft6_graph_form_negation_of_everything_is_impossible) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "integer",
    "enum": [ "x" ]
  })JSON")};

  // A dialect whose booleans are entries of the graph says that nothing
  // satisfies a schema by negating the one everything satisfies, and that is
  // the same impossible type the boolean used to generate
  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  using namespace sourcemeta::blaze;

  EXPECT_EQ(result.size(), 5);
  EXPECT_IR_ANY(result, 0, "/definitions/1");
  EXPECT_IR_IMPOSSIBLE(result, 2, "/definitions/0");
}

TEST(draft6_graph_form_closed_object_stays_closed) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "object",
    "properties": { "a": { "type": "string" } },
    "additionalProperties": false
  })JSON")};

  // What closes an object is an `additionalProperties` nothing can satisfy,
  // and the form says that from the entry an edge names rather than in place
  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  using namespace sourcemeta::blaze;

  EXPECT_EQ(result.size(), 9);
  EXPECT_IR_IMPOSSIBLE(result, 3, "/definitions/1");
  EXPECT_TRUE(std::holds_alternative<CodegenIRObject>(result.at(6)));

  const auto &object{std::get<CodegenIRObject>(result.at(6))};
  EXPECT_AS_STRING(object.pointer, "/definitions/0");
  EXPECT_EQ(object.members.size(), 1);
  EXPECT_TRUE(std::holds_alternative<bool>(object.additional));
  EXPECT_FALSE(std::get<bool>(object.additional));
}

TEST(draft6_graph_form_array_with_impossible_contains_generates) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "array",
    "contains": false
  })JSON")};

  // The entry that `contains` names says nothing about what the array holds,
  // so the array still generates from the items it does allow
  const auto result{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_walker,
      sourcemeta::core::schema_resolver, sourcemeta::blaze::default_compiler)};

  using namespace sourcemeta::blaze;

  EXPECT_EQ(result.size(), 7);
  EXPECT_IR_IMPOSSIBLE(result, 2, "/definitions/1");
  EXPECT_IR_ARRAY(result, 4, "/definitions/0", "/definitions/0/items");
}
