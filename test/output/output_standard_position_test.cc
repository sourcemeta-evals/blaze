#include <gtest/gtest.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer_position.h>
#include <sourcemeta/core/jsonschema.h>

#include <sstream>

namespace sourcemeta::blaze {

TEST(OutputStandardPosition, BasicError) {
  const sourcemeta::core::JSON schema = sourcemeta::core::parse_json(R"JSON({
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
})JSON");

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::core::default_schema_compiler)};

  const std::string instance_str = "123";
  std::istringstream stream{instance_str};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance = sourcemeta::core::parse_json(stream, std::ref(tracker));

  sourcemeta::blaze::Evaluator evaluator;

  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_FALSE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("errors"));
  EXPECT_TRUE(result.at("errors").is_array());
  EXPECT_EQ(result.at("errors").size(), 1);

  const auto &error = result.at("errors").at(0);
  EXPECT_TRUE(error.defines("instancePosition"));
  const auto &position = error.at("instancePosition");
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);

  // 123 is on line 1, column 1 (0-indexed? No, usually 1-indexed in editors,
  // let's check tracker implementation or just print it) Tracker usually uses
  // 1-based indexing for lines and columns. "123" -> line 1, col 1 to line 1,
  // col 3 (length 3)

  // Let's check what values we get.
  // std::cout << "Position: " << position.to_string() << std::endl;
}

TEST(OutputStandardPosition, BasicAnnotation) {
  const sourcemeta::core::JSON schema = sourcemeta::core::parse_json(R"JSON({
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "foo"
})JSON");

  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker,
      sourcemeta::core::schema_official_resolver,
      sourcemeta::core::default_schema_compiler)};

  const std::string instance_str = "123";
  std::istringstream stream{instance_str};
  sourcemeta::core::PointerPositionTracker tracker;
  const auto instance = sourcemeta::core::parse_json(stream, std::ref(tracker));

  sourcemeta::blaze::Evaluator evaluator;

  const auto result{
      sourcemeta::blaze::standard(evaluator, schema_template, instance, tracker,
                                  sourcemeta::blaze::StandardOutput::Basic)};

  EXPECT_TRUE(result.is_object());
  EXPECT_TRUE(result.defines("valid"));
  EXPECT_TRUE(result.at("valid").to_boolean());
  EXPECT_TRUE(result.defines("annotations"));
  EXPECT_TRUE(result.at("annotations").is_array());
  EXPECT_EQ(result.at("annotations").size(), 1);

  const auto &annotation = result.at("annotations").at(0);
  EXPECT_TRUE(annotation.defines("instancePosition"));
  const auto &position = annotation.at("instancePosition");
  EXPECT_TRUE(position.is_array());
  EXPECT_EQ(position.size(), 4);
}

} // namespace sourcemeta::blaze
