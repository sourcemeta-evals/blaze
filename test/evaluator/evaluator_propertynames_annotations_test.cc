#include <gtest/gtest.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/core/json.h>

#include "evaluator_utils.h"

TEST(Evaluator_propertyNames_annotations, title_annotation_suppressed) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "propertyNames": {
      "title": "Test"
    }
  })JSON")};

  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON({"foo": 1})JSON")};

  EVALUATE_WITH_TRACE_EXHAUSTIVE_SUCCESS(schema, instance, 1);

  EVALUATE_TRACE_PRE(0, LoopKeys, "/propertyNames", "#/propertyNames", "");
  EVALUATE_TRACE_POST_SUCCESS(0, LoopKeys, "/propertyNames", "#/propertyNames",
                              "");

  EVALUATE_TRACE_POST_DESCRIBE(instance, 0,
                               "The object properties \"foo\" were expected to "
                               "validate against the given subschema");
}

TEST(Evaluator_propertyNames_annotations,
     contentEncoding_annotation_suppressed) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "propertyNames": {
      "contentEncoding": "base64"
    }
  })JSON")};

  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON({"foo": 1})JSON")};

  EVALUATE_WITH_TRACE_EXHAUSTIVE_SUCCESS(schema, instance, 1);

  EVALUATE_TRACE_PRE(0, LoopKeys, "/propertyNames", "#/propertyNames", "");
  EVALUATE_TRACE_POST_SUCCESS(0, LoopKeys, "/propertyNames", "#/propertyNames",
                              "");

  EVALUATE_TRACE_POST_DESCRIBE(instance, 0,
                               "The object properties \"foo\" were expected to "
                               "validate against the given subschema");
}

TEST(Evaluator_propertyNames_annotations, format_annotation_suppressed) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "propertyNames": {
      "format": "email"
    }
  })JSON")};

  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON({"foo": 1})JSON")};

  EVALUATE_WITH_TRACE_EXHAUSTIVE_SUCCESS(schema, instance, 1);

  EVALUATE_TRACE_PRE(0, LoopKeys, "/propertyNames", "#/propertyNames", "");
  EVALUATE_TRACE_POST_SUCCESS(0, LoopKeys, "/propertyNames", "#/propertyNames",
                              "");

  EVALUATE_TRACE_POST_DESCRIBE(instance, 0,
                               "The object properties \"foo\" were expected to "
                               "validate against the given subschema");
}

TEST(Evaluator_propertyNames_annotations,
     regular_schema_still_emits_annotations) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "properties": {
      "foo": {
        "title": "Test"
      }
    }
  })JSON")};

  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON({"foo": 1})JSON")};

  EVALUATE_WITH_TRACE_EXHAUSTIVE_SUCCESS(schema, instance, 2);

  EVALUATE_TRACE_PRE(0, LoopProperties, "/properties", "#/properties", "");
  EVALUATE_TRACE_PRE(1, AnnotationEmit, "/properties/foo/title",
                     "#/properties/foo/title", "/foo");
  EVALUATE_TRACE_POST_SUCCESS(0, AnnotationEmit, "/properties/foo/title",
                              "#/properties/foo/title", "/foo");
  EVALUATE_TRACE_POST_SUCCESS(1, LoopProperties, "/properties", "#/properties",
                              "");

  EVALUATE_TRACE_POST_DESCRIBE(instance, 0,
                               "The annotation was attached to the instance");
  EVALUATE_TRACE_POST_DESCRIBE(instance, 1,
                               "The object properties \"foo\" were expected to "
                               "validate against the given subschemas");
}
