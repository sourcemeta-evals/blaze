#include <sourcemeta/core/test.h>

#include <sourcemeta/blaze/canonicalizer.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstddef>    // std::size_t
#include <filesystem> // std::filesystem
#include <iostream>   // std::cerr
#include <sstream>    // std::ostringstream
#include <string>     // std::string

#include "canonicalizer_test_utils.h"

namespace {

auto prettify(const sourcemeta::core::JSON &document) -> std::string {
  std::ostringstream stream;
  sourcemeta::core::prettify(document, stream);
  return stream.str();
}

// Canonicalisation decides where a keyword goes as much as whether it is there
// at all, so the order of the result is part of what a fixture blesses
auto expect_equal_with_ordering(const sourcemeta::core::JSON &actual,
                                const sourcemeta::core::JSON &expected)
    -> void {
  EXPECT_EQ(actual, expected);
  EXPECT_EQ(prettify(actual), prettify(expected));
}

auto canonicalize_schema(const sourcemeta::core::JSON &schema)
    -> sourcemeta::core::JSON {
  auto document{schema};
  sourcemeta::blaze::canonicalize(document, sourcemeta::core::schema_walker,
                                  canonicalizer_test_resolver);
  return document;
}

// A fixture is the document to canonicalise and the document it
// canonicalises to. The instances beside them say what each of the two means
auto check_shape(const sourcemeta::core::JSON &test) -> void {
  EXPECT_TRUE(test.is_object());
  EXPECT_TRUE(test.defines("schema"));
  EXPECT_TRUE(test.defines("expected"));
}

auto run_canonicalizer_test(const sourcemeta::core::JSON &test) -> void {
  check_shape(test);

  const auto document{canonicalize_schema(test.at("schema"))};
  expect_equal_with_ordering(document, test.at("expected"));
}

auto register_tests(const std::filesystem::path &directory) -> std::size_t {
  std::size_t count{0};
  for (const std::filesystem::directory_entry &entry :
       std::filesystem::recursive_directory_iterator{directory}) {
    if (!entry.is_regular_file() || entry.path().extension() != ".json") {
      continue;
    }

    const auto dialect{entry.path().parent_path().filename().string()};
    std::ostringstream name;
    for (const auto character : entry.path().stem().string()) {
      name << (character == '-' ? '_' : character);
    }

    const auto test{sourcemeta::core::read_json(entry.path())};
    sourcemeta::core::test_register(
        "CanonicalizerSuite_" + dialect, name.str(), __FILE__, __LINE__,
        [test]() -> void { run_canonicalizer_test(test); });
    count += 1;
  }

  return count;
}

} // namespace

auto main(int argc, char **argv) -> int {
  const auto count{
      register_tests(std::filesystem::path{CANONICALIZER_SUITE_PATH})};
  // A fixture in the wrong place, or with the wrong extension, would otherwise
  // never run and nobody would notice
  if (count == 0) {
    std::cerr << "No canonicalizer fixtures found at "
              << CANONICALIZER_SUITE_PATH << "\n";
    return 1;
  }

  return sourcemeta::core::test_run(argc, argv);
}
