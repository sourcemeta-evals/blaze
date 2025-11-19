#include <gtest/gtest.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#define EXPECT_ANNOTATION_COUNT(output, expected_count)                        \
  EXPECT_EQ(output.annotations().size(), (expected_count));

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

TEST(Compiler_output_simple_repro, contains_drops_failed_annotations) {
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

  EXPECT_TRUE(result);

  // We expect only one annotation for the element that passed (42 at index 1)
  // The bug causes annotations for 0 and 2 to be present as well.

  // Check that we have the correct annotation for index 1
  EXPECT_ANNOTATION_ENTRY(output, "/1", "/contains/title", "#/contains/title",
                          1);

  // These assertions should fail if the bug is present
  // We want to verify that annotations for /0 and /2 are NOT present

  const auto instance_location_0{sourcemeta::core::to_pointer("/0")};
  const auto evaluate_path{sourcemeta::core::to_pointer("/contains/title")};

  EXPECT_FALSE(output.annotations().contains(
      {sourcemeta::core::to_weak_pointer(instance_location_0),
       sourcemeta::core::to_weak_pointer(evaluate_path), "#/contains/title"}));

  const auto instance_location_2{sourcemeta::core::to_pointer("/2")};
  EXPECT_FALSE(output.annotations().contains(
      {sourcemeta::core::to_weak_pointer(instance_location_2),
       sourcemeta::core::to_weak_pointer(evaluate_path), "#/contains/title"}));

  // Total annotations should be 1 (plus the one for "contains" itself on the
  // root?) The "contains" keyword itself produces an annotation which is the
  // index of the first match or list of matches depending on draft? In 2020-12,
  // "contains" does not produce an annotation itself unless
  // minContains/maxContains are involved? Actually, let's check the CLI output
  // in the description: annotation: 1
  //   at instance location ""
  //   at evaluate path "/contains"
  // annotation: "Test"
  //   at instance location "/0" ...

  // So there is an annotation for "/contains" at root.
  // And annotations for "title" at indices.

  // So we expect:
  // 1. Annotation at "" path "/contains" (value 1, meaning index 1 matched? or
  // count?)
  //    The CLI output says "annotation: 1".
  // 2. Annotation at "/1" path "/contains/title" (value "Test")

  // Total annotations should be 2.
  // If bug is present, we will have 4 annotations (root + 0 + 1 + 2).

  EXPECT_ANNOTATION_COUNT(output, 2);
}
