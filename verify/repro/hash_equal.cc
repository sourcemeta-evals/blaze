// Run: built alongside probe by verify/repro/build_probe.sh; then
//   ./hash_equal <name1> <name2>
// Prints the PropertyHashJSON hash of each property name (as used by the
// evaluator's exact-set matching), whether it is a perfect hash, and whether
// the two hashes compare equal.
#include <sourcemeta/core/json.h>

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

using Hasher = sourcemeta::core::PropertyHashJSON<sourcemeta::core::JSON::String>;

static auto print_hash(const char *label, const std::string &name,
                       const Hasher &hasher) -> void {
  const auto hash{hasher(name)};
  unsigned char bytes[sizeof(hash)];
  std::memcpy(bytes, &hash, sizeof(hash));
  std::cout << label << " size=" << name.size() << " perfect="
            << hasher.is_perfect(hash) << " hash=";
  for (const auto byte : bytes) {
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<unsigned>(byte);
  }
  std::cout << std::dec << "\n";
}

auto main(int argc, char **argv) -> int {
  if (argc != 3) {
    std::cerr << "usage: hash_equal <name1> <name2>\n";
    return EXIT_FAILURE;
  }

  const Hasher hasher;
  const std::string left{argv[1]};
  const std::string right{argv[2]};
  print_hash("name1", left, hasher);
  print_hash("name2", right, hasher);
  std::cout << "equal_hash=" << (hasher(left) == hasher(right))
            << " same_prefix31=" << (left.substr(0, 31) == right.substr(0, 31))
            << " same_size=" << (left.size() == right.size())
            << " same_last=" << (left.back() == right.back())
            << " distinct_names=" << (left != right) << "\n";
  return EXIT_SUCCESS;
}
