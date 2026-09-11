// Appended to test/evaluator/evaluator_2019_09_test.cc by verify/repro/c3_demo.sh.
// The two annotation-enabled fast-mode 2019-09 cases missing from the committed
// suite: (1) wrong property name on an exact property-set schema, (2) the valid
// control for the same schema. Mirrors the 2020-12 pair the branch did add
// (properties_closed_type_strict_undeclared_fast_annotations /
// properties_closed_type_strict_valid_fast_annotations).

#include <iostream>
#include <string>
#include <unordered_set>

static auto c3_probe_types(const sourcemeta::blaze::Template &compiled)
    -> std::string {
  std::string types;
  for (const auto &instruction : compiled.targets.front()) {
    types += sourcemeta::blaze::InstructionNames[static_cast<std::size_t>(
        instruction.type)];
    types += ",";
  }
  return types;
}

static const char *const C3_SCHEMA = R"JSON({
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "type": "object",
  "properties": {
    "notification_webhook_endpoint_url_for_production_alerts": { "type": "string" },
    "notification_webhook_endpoint_url_for_staging_alerts": { "type": "string" }
  },
  "required": [
    "notification_webhook_endpoint_url_for_production_alerts",
    "notification_webhook_endpoint_url_for_staging_alerts"
  ],
  "additionalProperties": false
})JSON";

TEST(c3_probe_2019_09_wrong_name_fast_annotations) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(C3_SCHEMA)};
  const sourcemeta::core::JSON instance{sourcemeta::core::parse_json(R"JSON({
    "a_property_name_we_never_declared_anywhere_in_the_schema": "foo",
    "another_property_name_that_should_definitely_be_rejected": "bar"
  })JSON")};
  sourcemeta::blaze::Tweaks tweaks;
  tweaks.annotations =
      std::unordered_set<sourcemeta::core::JSON::StringView>{"properties"};
  const auto compiled{sourcemeta::blaze::compile(
      schema, sourcemeta::blaze::schema_walker,
      sourcemeta::blaze::schema_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation, "", "", "", tweaks)};
  sourcemeta::blaze::Evaluator evaluator;
  const bool result{evaluator.validate(compiled, instance)};
  std::cerr << "[c3-probe] 2019-09 fast+annotations{properties} instructions: "
            << c3_probe_types(compiled) << "\n";
  std::cerr << "[c3-probe] 2019-09 wrong-name -> " << (result ? "true" : "false")
            << " (expected false)\n";
  EXPECT_FALSE(result);
}

TEST(c3_probe_2019_09_valid_control_fast_annotations) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(C3_SCHEMA)};
  const sourcemeta::core::JSON instance{sourcemeta::core::parse_json(R"JSON({
    "notification_webhook_endpoint_url_for_production_alerts": "foo",
    "notification_webhook_endpoint_url_for_staging_alerts": "bar"
  })JSON")};
  sourcemeta::blaze::Tweaks tweaks;
  tweaks.annotations =
      std::unordered_set<sourcemeta::core::JSON::StringView>{"properties"};
  const auto compiled{sourcemeta::blaze::compile(
      schema, sourcemeta::blaze::schema_walker,
      sourcemeta::blaze::schema_resolver,
      sourcemeta::blaze::default_schema_compiler,
      sourcemeta::blaze::Mode::FastValidation, "", "", "", tweaks)};
  sourcemeta::blaze::Evaluator evaluator;
  const bool result{evaluator.validate(compiled, instance)};
  std::cerr << "[c3-probe] 2019-09 valid control -> "
            << (result ? "true" : "false") << " (expected true)\n";
  EXPECT_TRUE(result);
}
