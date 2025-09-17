#include <gtest/gtest.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#define EXPECT_ANNOTATION_COUNT(output, expected_count)                        \
  EXPECT_EQ(output.annotations().size(), (expected_count));

#define EXPECT_ANNOTATION_VALUE(                                               \
    output, expected_instance_location, expected_evaluate_path,                \
    expected_schema_location, expected_entry_index, expected_value)            \
  {                                                                            \
    const auto instance_location{                                              \
        sourcemeta::core::to_pointer(expected_instance_location)};             \
    const auto evaluate_path{                                                  \
        sourcemeta::core::to_pointer(expected_evaluate_path)};                 \
    EXPECT_EQ(output.annotations()                                             \
                  .at({sourcemeta::core::to_weak_pointer(instance_location),   \
                       sourcemeta::core::to_weak_pointer(evaluate_path),       \
                       (expected_schema_location)})                            \
                  .at(expected_entry_index),                                   \
              (expected_value));                                               \
  }

#define EXPECT_ANNOTATION_ENTRY(                                               \
    output, expected_instance_location, expected_evaluate_path,                \
    expected_schema_location, expected_entry_count)                            \
  {                                                                            \
    const auto instance_location{                                              \
        sourcemeta::core::to_pointer(expected_instance_location)};             \
    const auto evaluate_path{                                                  \
        sourcemeta::core::to_pointer(expected_evaluate_path)};                 \
    EXPECT_TRUE(output.annotations().contains(                                 \
        {sourcemeta::core::to_weak_pointer(instance_location),                 \
         sourcemeta::core::to_weak_pointer(evaluate_path),                     \
         (expected_schema_location)}));                                        \
    EXPECT_EQ(output.annotations()                                             \
                  .at({sourcemeta::core::to_weak_pointer(instance_location),   \
                       sourcemeta::core::to_weak_pointer(evaluate_path),       \
                       (expected_schema_location)})                            \
                  .size(),                                                     \
              (expected_entry_count));                                         \
  }

#define EXPECT_NO_ANNOTATION_ENTRY(output, expected_instance_location,         \
                                   expected_evaluate_path,                     \
                                   expected_schema_location)                   \
  {                                                                            \
    const auto instance_location{                                              \
        sourcemeta::core::to_pointer(expected_instance_location)};             \
    const auto evaluate_path{                                                  \
        sourcemeta::core::to_pointer(expected_evaluate_path)};                 \
    EXPECT_FALSE(output.annotations().contains(                                \
        {sourcemeta::core::to_weak_pointer(instance_location),                 \
         sourcemeta::core::to_weak_pointer(evaluate_path),                     \
         (expected_schema_location)}));                                        \
  }

TEST(Compiler_output_simple, contains_mixed_success_failure_annotations) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "contains": { 
      "type": "number",
      "title": "Test" 
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON([ "foo", 42, true ])JSON")};

  sourcemeta::blaze::SimpleOutput output{instance};
  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      evaluator.validate(schema_template, instance, std::ref(output))};

  // Clean up annotations from failed contains subschemas
  output.cleanup_failed_contains_annotations();

  EXPECT_TRUE(result);

  // Should have contains annotation at root level with successful index
  EXPECT_ANNOTATION_ENTRY(output, "", "/contains", "#/contains", 1);
  EXPECT_ANNOTATION_VALUE(output, "", "/contains", "#/contains", 0,
                          sourcemeta::core::JSON{1});

  // Title annotations should only exist for successful items
  EXPECT_NO_ANNOTATION_ENTRY(output, "/0", "/contains/title",
                             "#/contains/title");
  EXPECT_ANNOTATION_ENTRY(output, "/1", "/contains/title", "#/contains/title",
                          1);
  EXPECT_NO_ANNOTATION_ENTRY(output, "/2", "/contains/title",
                             "#/contains/title");
}

TEST(Compiler_output_simple, contains_all_success_annotations) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "contains": { 
      "type": "number",
      "title": "Test" 
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON([ 1, 42, 3 ])JSON")};

  sourcemeta::blaze::SimpleOutput output{instance};
  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      evaluator.validate(schema_template, instance, std::ref(output))};

  // Clean up annotations from failed contains subschemas
  output.cleanup_failed_contains_annotations();

  EXPECT_TRUE(result);

  // Should have contains annotation at root level with all successful indices
  EXPECT_ANNOTATION_ENTRY(output, "", "/contains", "#/contains", 3);
  EXPECT_ANNOTATION_VALUE(output, "", "/contains", "#/contains", 0,
                          sourcemeta::core::JSON{0});
  EXPECT_ANNOTATION_VALUE(output, "", "/contains", "#/contains", 1,
                          sourcemeta::core::JSON{1});
  EXPECT_ANNOTATION_VALUE(output, "", "/contains", "#/contains", 2,
                          sourcemeta::core::JSON{2});

  // Title annotations should exist for all successful items
  EXPECT_ANNOTATION_ENTRY(output, "/0", "/contains/title", "#/contains/title",
                          1);
  EXPECT_ANNOTATION_ENTRY(output, "/1", "/contains/title", "#/contains/title",
                          1);
  EXPECT_ANNOTATION_ENTRY(output, "/2", "/contains/title", "#/contains/title",
                          1);
}

TEST(Compiler_output_simple, contains_all_failure_annotations) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "contains": { 
      "type": "number",
      "title": "Test" 
    }
  })JSON")};

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::Exhaustive)};

  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON([ "foo", "bar", true ])JSON")};

  sourcemeta::blaze::SimpleOutput output{instance};
  sourcemeta::blaze::Evaluator evaluator;
  const auto result{
      evaluator.validate(schema_template, instance, std::ref(output))};

  // Clean up annotations from failed contains subschemas
  output.cleanup_failed_contains_annotations();

  EXPECT_FALSE(result);

  // No contains annotation should exist since no items matched
  EXPECT_NO_ANNOTATION_ENTRY(output, "", "/contains", "#/contains");

  // No title annotations should exist since all items failed
  EXPECT_NO_ANNOTATION_ENTRY(output, "/0", "/contains/title",
                             "#/contains/title");
  EXPECT_NO_ANNOTATION_ENTRY(output, "/1", "/contains/title",
                             "#/contains/title");
  EXPECT_NO_ANNOTATION_ENTRY(output, "/2", "/contains/title",
                             "#/contains/title");
}
