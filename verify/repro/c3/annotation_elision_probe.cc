// Probe: fast-mode compilation of a closed single-type object schema with
// `required`, with and without `properties` annotations enabled via Tweaks.
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/core/json.h>

#include <iostream>
#include <optional>
#include <unordered_set>

static auto run(const std::optional<sourcemeta::blaze::Tweaks> &tweaks,
                const char *label) -> void {
  const auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": { "a": { "type": "string" }, "b": { "type": "string" } },
    "required": [ "a", "b" ],
    "additionalProperties": false
  })JSON")};
  const auto compiled{sourcemeta::blaze::compile(
      schema, sourcemeta::blaze::schema_walker,
      sourcemeta::blaze::schema_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation, "", "", "", tweaks)};
  std::cout << "### " << label << "\n";
  std::cout << "template: " << sourcemeta::blaze::to_json(compiled) << "\n";
  sourcemeta::blaze::Evaluator evaluator;
  const char *instances[] = {R"({})", R"({"a":"x"})", R"({"a":"x","b":"y"})",
                             R"({"a":"x","b":"y","c":"z"})"};
  for (const auto *text : instances) {
    const auto instance{sourcemeta::core::parse_json(text)};
    std::cout << text << " -> "
              << (evaluator.validate(compiled, instance) ? "PASS" : "FAIL")
              << "\n";
  }
}

auto main() -> int {
  run(std::nullopt, "fast, default tweaks (no annotations)");
  sourcemeta::blaze::Tweaks tweaks;
  tweaks.annotations =
      std::unordered_set<sourcemeta::core::JSON::StringView>{"properties"};
  run(tweaks, "fast, tweaks.annotations={\"properties\"}");
  return 0;
}
