// Run: see verify/repro/build_probe.sh (builds against a Blaze `build/dist`
// install), then: ./probe <schema.json> <instance.json> [fast|exhaustive]
//   [--annotations kw1,kw2] [--callback] [--dump]
//
// Prints `valid=<0|1>` for the requested mode. With `--callback`, validation
// runs through the callback API and every emitted annotation is printed as
// `annotation result=<0|1> <evaluate_path> <instance_location> <value>`
// (result=1 marks a successful annotation). With `--dump`, the
// instruction types of the compiled template are printed (for typed-hash
// instructions, the ordered declared property names follow in brackets).
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

static auto read_file(const char *path) -> sourcemeta::core::JSON {
  std::ifstream stream{path};
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return sourcemeta::core::parse_json(buffer.str());
}

static auto dump(const sourcemeta::blaze::Instructions &instructions,
                 const std::size_t depth) -> void {
  for (const auto &instruction : instructions) {
    std::cout << std::string(depth * 2, ' ')
              << sourcemeta::blaze::InstructionNames[static_cast<std::size_t>(
                     instruction.type)];
    if (std::holds_alternative<sourcemeta::blaze::ValueTypedHashes>(
            instruction.value)) {
      const auto &hashes{
          std::get<sourcemeta::blaze::ValueTypedHashes>(instruction.value)};
      std::cout << " [";
      for (std::size_t index = 0; index < hashes.second.first.size();
           ++index) {
        std::cout << (index == 0 ? "" : ", ")
                  << sourcemeta::core::JSON{hashes.second.first[index].second};
      }
      std::cout << "]";
    } else if (std::holds_alternative<sourcemeta::blaze::ValueStringHashes>(
                   instruction.value)) {
      const auto &hashes{
          std::get<sourcemeta::blaze::ValueStringHashes>(instruction.value)};
      std::cout << " [";
      for (std::size_t index = 0; index < hashes.first.size(); ++index) {
        std::cout << (index == 0 ? "" : ", ")
                  << sourcemeta::core::JSON{hashes.first[index].second};
      }
      std::cout << "]";
    } else if (std::holds_alternative<sourcemeta::blaze::ValueTypedProperties>(
                   instruction.value)) {
      const auto &properties{
          std::get<sourcemeta::blaze::ValueTypedProperties>(
              instruction.value)};
      std::cout << " [";
      bool first{true};
      for (const auto &entry : properties.second) {
        std::cout << (first ? "" : ", ")
                  << sourcemeta::core::JSON{entry.first};
        first = false;
      }
      std::cout << "]";
    }
    std::cout << "\n";
    dump(instruction.children, depth + 1);
  }
}

auto main(int argc, char **argv) -> int {
  if (argc < 3) {
    std::cerr << "usage: probe <schema.json> <instance.json> [fast|exhaustive]"
                 " [--annotations kw1,kw2] [--callback] [--dump]\n";
    return EXIT_FAILURE;
  }

  const auto schema{read_file(argv[1])};
  const auto instance{read_file(argv[2])};
  auto mode{sourcemeta::blaze::Mode::FastValidation};
  bool use_callback{false};
  bool do_dump{false};
  std::optional<sourcemeta::blaze::Tweaks> tweaks;
  std::vector<std::string> annotation_keywords;

  for (int index = 3; index < argc; index++) {
    const std::string_view argument{argv[index]};
    if (argument == "fast") {
      mode = sourcemeta::blaze::Mode::FastValidation;
    } else if (argument == "exhaustive") {
      mode = sourcemeta::blaze::Mode::Exhaustive;
    } else if (argument == "--callback") {
      use_callback = true;
    } else if (argument == "--dump") {
      do_dump = true;
    } else if (argument == "--annotations" && index + 1 < argc) {
      std::stringstream list{argv[++index]};
      std::string keyword;
      while (std::getline(list, keyword, ',')) {
        annotation_keywords.push_back(keyword);
      }
    } else {
      std::cerr << "unknown argument: " << argument << "\n";
      return EXIT_FAILURE;
    }
  }

  if (!annotation_keywords.empty()) {
    tweaks.emplace();
    tweaks->annotations.emplace();
    for (const auto &keyword : annotation_keywords) {
      tweaks->annotations->insert(keyword);
    }
  }

  const auto compiled{sourcemeta::blaze::compile(
      schema, sourcemeta::blaze::schema_walker,
      sourcemeta::blaze::schema_resolver,
      sourcemeta::blaze::default_schema_compiler, mode, "", "", "", tweaks)};

  if (do_dump) {
    std::cout << "template:\n";
    for (const auto &target : compiled.targets) {
      dump(target, 1);
    }
  }

  sourcemeta::blaze::Evaluator evaluator;
  bool valid{false};
  if (use_callback) {
    valid = evaluator.validate(
        compiled, instance,
        [](const sourcemeta::blaze::EvaluationType type, const bool result,
           const sourcemeta::blaze::Instruction &,
           const sourcemeta::blaze::InstructionExtra &,
           const sourcemeta::core::WeakPointer &evaluate_path,
           const sourcemeta::core::WeakPointer &instance_location,
           const sourcemeta::core::JSON &annotation) {
          if (type == sourcemeta::blaze::EvaluationType::Post &&
              !annotation.is_null()) {
            std::cout << "annotation result=" << result << " "
                      << sourcemeta::core::to_string(evaluate_path) << " "
                      << sourcemeta::core::to_string(instance_location) << " ";
            sourcemeta::core::stringify(annotation, std::cout);
            std::cout << "\n";
          }
        });
  } else {
    valid = evaluator.validate(compiled, instance);
  }

  std::cout << "mode=" << (mode == sourcemeta::blaze::Mode::FastValidation
                               ? "fast"
                               : "exhaustive")
            << " callback=" << use_callback << " valid=" << valid << "\n";
  return EXIT_SUCCESS;
}
