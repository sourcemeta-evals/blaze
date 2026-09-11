// Appended to test/evaluator/evaluator_test.cc by verify/repro/c2_mutation_demo.sh.
// This is the fast-mode wrong-property-name case with `properties` annotations
// enabled through Tweaks.annotations that the committed C++ tests lack: an
// exact property-set schema (two required string properties, no additional
// properties) and an instance with the same property count and value types but
// different names. Mirrors corpus scenario properties_exactly_type_strict_wrong_names.

#include <iostream>
#include <string>
#include <unordered_set>

static auto c2_probe_types(const sourcemeta::blaze::Template &compiled)
    -> std::string {
  std::string types;
  for (const auto &instruction : compiled.targets.front()) {
    types += sourcemeta::blaze::InstructionNames[static_cast<std::size_t>(
        instruction.type)];
    types += ",";
  }
  return types;
}

static auto c2_probe_compile(const sourcemeta::core::JSON &schema,
                             const sourcemeta::blaze::Mode mode,
                             const sourcemeta::blaze::Tweaks &tweaks)
    -> sourcemeta::blaze::Template {
  return sourcemeta::blaze::compile(
      schema, sourcemeta::blaze::schema_walker,
      sourcemeta::blaze::schema_resolver,
      sourcemeta::blaze::default_schema_compiler, mode, "", "", "", tweaks);
}

TEST(c2_probe_fast_wrong_property_name_with_properties_annotations) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "required": [ "foo", "bar" ],
    "additionalProperties": false,
    "properties": {
      "foo": { "type": "string" },
      "bar": { "type": "string" }
    }
  })JSON")};

  const sourcemeta::core::JSON wrong_name{sourcemeta::core::parse_json(
      R"JSON({ "foo": "1", "baz": "2" })JSON")};
  const sourcemeta::core::JSON valid{sourcemeta::core::parse_json(
      R"JSON({ "foo": "1", "bar": "2" })JSON")};

  sourcemeta::blaze::Tweaks annotations;
  annotations.annotations =
      std::unordered_set<sourcemeta::core::JSON::StringView>{"properties"};
  sourcemeta::blaze::Tweaks plain;

  const auto fast_annotated{c2_probe_compile(
      schema, sourcemeta::blaze::Mode::FastValidation, annotations)};
  const auto fast_plain{
      c2_probe_compile(schema, sourcemeta::blaze::Mode::FastValidation, plain)};
  const auto exhaustive_annotated{c2_probe_compile(
      schema, sourcemeta::blaze::Mode::Exhaustive, annotations)};

  sourcemeta::blaze::Evaluator evaluator;
  const bool fa_wrong{evaluator.validate(fast_annotated, wrong_name)};
  const bool fa_valid{evaluator.validate(fast_annotated, valid)};
  const bool fp_wrong{evaluator.validate(fast_plain, wrong_name)};
  const bool fp_valid{evaluator.validate(fast_plain, valid)};
  const bool ea_wrong{evaluator.validate(exhaustive_annotated, wrong_name)};
  const bool ea_valid{evaluator.validate(exhaustive_annotated, valid)};

  const auto text{[](const bool value) { return value ? "true" : "false"; }};
  std::cerr << "[c2-probe] fast+annotations{properties} instructions: "
            << c2_probe_types(fast_annotated) << "\n";
  std::cerr << "[c2-probe] fast+annotations{properties}: wrong-name -> "
            << text(fa_wrong) << "; valid -> " << text(fa_valid) << "\n";
  std::cerr << "[c2-probe] fast, no annotations instructions: "
            << c2_probe_types(fast_plain) << "\n";
  std::cerr << "[c2-probe] fast, no annotations: wrong-name -> "
            << text(fp_wrong) << "; valid -> " << text(fp_valid) << "\n";
  std::cerr << "[c2-probe] exhaustive+annotations{properties}: wrong-name -> "
            << text(ea_wrong) << "; valid -> " << text(ea_valid) << "\n";

  EXPECT_TRUE(fa_valid);
  EXPECT_TRUE(fp_valid);
  EXPECT_TRUE(ea_valid);
  EXPECT_FALSE(fp_wrong);
  EXPECT_FALSE(ea_wrong);
  EXPECT_FALSE(fa_wrong);
}
