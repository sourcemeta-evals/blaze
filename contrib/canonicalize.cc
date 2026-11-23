#include <sourcemeta/blaze/canonicalizer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/json.h>

#include <cstdlib>     // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>    // std::cerr, std::cout
#include <string_view> // std::string_view

auto main(int argc, char *argv[]) -> int {
  if (argc != 2 && argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <schema.json> [dialect]\n";
    return EXIT_FAILURE;
  }

  auto document{sourcemeta::core::read_json(argv[1])};

  // A document that is nothing but a boolean, or nothing but a reference, has
  // nowhere to name the dialect it is written in, so the dialect can be handed
  // in alongside it instead
  const std::string_view default_dialect{argc == 3 ? argv[2] : ""};

  sourcemeta::blaze::canonicalize(document, sourcemeta::core::schema_walker,
                                  sourcemeta::core::schema_resolver,
                                  default_dialect);

  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::Locations, document,
      sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver};
  sourcemeta::core::schema_format(document, frame);
  sourcemeta::core::prettify(document, std::cout);
  std::cout << "\n";

  return EXIT_SUCCESS;
}
