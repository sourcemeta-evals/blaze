// Run: append this file to test/evaluator/evaluator_2020_12_test.cc on a checkout of evalon/blaze-conf-423d1e31 (and optionally apply c4_instrumentation.patch), then
//      cmake -S . -B build -DBLAZE_TESTS=ON && cmake --build build --target sourcemeta_blaze_evaluator_unit && ./build/test/evaluator/sourcemeta_blaze_evaluator_unit --filter evaluator_2020_12.zz_verify_c4_probe
#include <iostream>
TEST(zz_verify_c4_probe) {
  const sourcemeta::core::JSON schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {"b": {"type": "string"}, "aa": {"type": "string"}},
    "required": ["b", "aa"],
    "additionalProperties": false
  })JSON")};
  const sourcemeta::core::JSON instance{
      sourcemeta::core::parse_json(R"JSON({"aa": "x", "b": 1})JSON")};
  const sourcemeta::core::JSON ordered{
      sourcemeta::core::parse_json(R"JSON({"b": 1, "aa": "x"})JSON")};
  for (const auto mode : {sourcemeta::blaze::Mode::FastValidation,
                          sourcemeta::blaze::Mode::Exhaustive}) {
    const auto compiled_schema{sourcemeta::blaze::compile(
        schema, sourcemeta::blaze::schema_walker,
        sourcemeta::blaze::schema_resolver,
        sourcemeta::blaze::default_schema_compiler, mode)};
    std::cerr << "[C4-PROBE] mode="
              << (mode == sourcemeta::blaze::Mode::FastValidation ? "fast" : "exhaustive")
              << " first_instruction="
              << sourcemeta::blaze::InstructionNames[std::to_underlying(
                     compiled_schema.targets.front().front().type)]
              << "\n";
    sourcemeta::blaze::Evaluator probe_evaluator;
    std::cerr << "[C4-PROBE] callbackless validate({\"aa\":\"x\",\"b\":1}) => "
              << (probe_evaluator.validate(compiled_schema, instance) ? "true" : "false") << "\n";
    EVALUATE_WITH_TRACE(compiled_schema, instance, trace_pre.size());
    std::cerr << "[C4-PROBE] callback validate({\"aa\":\"x\",\"b\":1}) => "
              << (result ? "true" : "false") << "\n";
    std::cerr << "[C4-PROBE] control callbackless validate({\"b\":1,\"aa\":\"x\"}) => "
              << (probe_evaluator.validate(compiled_schema, ordered) ? "true" : "false") << "\n";
    // The instance is invalid under JSON Schema ("b" must be a string)
    EXPECT_FALSE(probe_evaluator.validate(compiled_schema, instance));
  }
}
