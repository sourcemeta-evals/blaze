#include <sourcemeta/blaze/canonicalizer.h>
#include <sourcemeta/blaze/codegen.h>
#include <sourcemeta/core/jsonschema.h>

#include <algorithm>     // std::ranges::sort
#include <cassert>       // assert
#include <functional>    // std::reference_wrapper
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

#include "codegen_default_compiler.h"

namespace {

auto is_validation_subschema(
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver) -> bool {
  if (!location.parent.has_value()) {
    return false;
  }

  const auto &parent{location.parent.value()};
  if (parent.size() >= location.pointer.size()) {
    return false;
  }

  const auto &keyword_token{location.pointer.at(parent.size())};
  if (!keyword_token.is_property()) {
    return false;
  }

  const auto parent_location{frame.traverse(parent)};
  if (!parent_location.has_value()) {
    return false;
  }

  const auto &vocabularies{
      frame.vocabularies(parent_location.value().get(), resolver)};
  const auto &walker_result{walker(keyword_token.to_property(), vocabularies)};
  using Type = sourcemeta::core::SchemaKeywordType;
  if (walker_result.type == Type::ApplicatorValueTraverseAnyPropertyKey ||
      walker_result.type == Type::ApplicatorValueTraverseAnyItem) {
    return true;
  }

  return is_validation_subschema(frame, parent_location.value().get(), walker,
                                 resolver);
}

using Subschemas = std::vector<
    std::reference_wrapper<const sourcemeta::core::SchemaFrame::Location>>;

/// The innermost subschema a given one sits inside, or nothing when it is the
/// document itself
auto enclosing_subschema(const Subschemas &subschemas,
                         const sourcemeta::core::WeakPointer &pointer)
    -> const sourcemeta::core::SchemaFrame::Location * {
  const sourcemeta::core::SchemaFrame::Location *result{nullptr};
  for (const auto &candidate : subschemas) {
    const auto &other{candidate.get()};
    if (pointer.size() > other.pointer.size() &&
        pointer.starts_with(other.pointer) &&
        (result == nullptr || other.pointer.size() > result->pointer.size())) {
      result = &other;
    }
  }

  return result;
}

/// Whether the keyword leading from one subschema to another says only what an
/// instance must satisfy, never what it looks like
auto edge_only_validates(const sourcemeta::core::SchemaFrame &frame,
                         const sourcemeta::core::SchemaWalker &walker,
                         const sourcemeta::core::SchemaResolver &resolver,
                         const sourcemeta::core::SchemaFrame::Location &parent,
                         const sourcemeta::core::SchemaFrame::Location &child)
    -> bool {
  const auto &token{child.pointer.at(parent.pointer.size())};
  if (!token.is_property()) {
    return false;
  }

  const auto &vocabularies{frame.vocabularies(parent, resolver)};
  using Type = sourcemeta::core::SchemaKeywordType;
  switch (walker(token.to_property(), vocabularies).type) {
    case Type::ApplicatorValueTraverseAnyItem:
    case Type::ApplicatorValueTraverseAnyPropertyKey:
      return true;
    default:
      return false;
  }
}

/// What each edge of the document is for, by where the edge sits
using EdgeKinds = std::unordered_map<sourcemeta::core::WeakPointer, bool,
                                     sourcemeta::core::WeakPointer::Hasher,
                                     sourcemeta::core::WeakPointer::Comparator>;

auto edge_kinds(const sourcemeta::core::SchemaFrame &frame,
                const sourcemeta::core::SchemaWalker &walker,
                const sourcemeta::core::SchemaResolver &resolver,
                const Subschemas &subschemas) -> EdgeKinds {
  EdgeKinds result;
  for (const auto &candidate : subschemas) {
    const auto &subschema{candidate.get()};
    const auto *node{enclosing_subschema(subschemas, subschema.pointer)};
    if (node != nullptr) {
      result.emplace(
          subschema.pointer,
          edge_only_validates(frame, walker, resolver, *node, subschema));
    }
  }

  return result;
}

/// Whether anything that reaches a subschema says what an instance looks like,
/// for each subschema a reference reaches at all. While a dialect keeps its
/// subschemas nested, the keywords above one are enough to tell what it is
/// for. Once a dialect lifts every subschema into one container, nothing sits
/// under anything, and the edges that reach a node are what say it
using Arrivals = std::unordered_map<sourcemeta::core::WeakPointer, bool,
                                    sourcemeta::core::WeakPointer::Hasher,
                                    sourcemeta::core::WeakPointer::Comparator>;

auto arrivals(const sourcemeta::core::SchemaFrame &frame,
              const EdgeKinds &kinds) -> Arrivals {
  Arrivals result;
  frame.for_each_reference_into(
      sourcemeta::core::EMPTY_WEAK_POINTER,
      [&frame, &kinds, &result](
          const sourcemeta::core::SchemaReferenceType,
          const sourcemeta::core::WeakPointer &source,
          const sourcemeta::core::SchemaFrame::Reference &reference) -> void {
        const auto destination{frame.traverse(reference.destination)};
        if (!destination.has_value()) {
          return;
        }

        // A reference sits inside the subschema that stands for the edge, as
        // the keyword that spells it
        auto edge{sourcemeta::core::to_pointer(source)};
        edge.pop_back();
        const auto kind{kinds.find(sourcemeta::core::to_weak_pointer(edge))};
        const auto to_describe{kind == kinds.cend() || !kind->second};
        const auto [entry, inserted] =
            result.emplace(destination.value().get().pointer, to_describe);
        if (!inserted && to_describe) {
          entry->second = true;
        }
      });

  return result;
}

// The dialect a schema declares, falling back to the given default. Unlike the
// equivalent helper in bundling, this one hands back a copy rather than a view,
// as canonicalisation goes on to rewrite the very document it reads from
auto declared_dialect(const sourcemeta::core::JSON &schema,
                      const std::string_view default_dialect)
    -> sourcemeta::core::JSON::String {
  if (!schema.is_object()) {
    return sourcemeta::core::JSON::String{default_dialect};
  }

  const auto *dialect{schema.try_at("$schema")};
  return (dialect != nullptr && dialect->is_string())
             ? dialect->to_string()
             : sourcemeta::core::JSON::String{default_dialect};
}

} // anonymous namespace

namespace sourcemeta::blaze {

auto compile(const sourcemeta::core::JSON &input,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const CodegenCompiler &compiler,
             const std::string_view default_dialect,
             const std::string_view default_id) -> CodegenIRResult {
  // --------------------------------------------------------------------------
  // (1) Bundle the schema to resolve external references
  // --------------------------------------------------------------------------

  sourcemeta::core::SchemaBundleOptions bundle_options;
  bundle_options.mode = sourcemeta::core::SchemaBundleOptions::Mode::References;
  auto schema{sourcemeta::core::schema_bundle(
      input, walker, resolver, default_dialect, default_id, bundle_options)};

  // --------------------------------------------------------------------------
  // (2) Canonicalize the schema for easier analysis
  // --------------------------------------------------------------------------

  // Canonicalisation can collapse an unsatisfiable schema into a boolean,
  // which has no room for the dialect the document declares, so hold on to
  // that dialect for the framing that follows
  const auto dialect{declared_dialect(schema, default_dialect)};

  sourcemeta::blaze::canonicalize(schema, walker, resolver, dialect,
                                  default_id);

  // --------------------------------------------------------------------------
  // (3) Frame the resulting schema with instance location information
  // --------------------------------------------------------------------------

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References,
      schema,
      walker,
      resolver,
      dialect,
      default_id};

  // --------------------------------------------------------------------------
  // (4) Convert every subschema into a code generation object
  // --------------------------------------------------------------------------

  std::unordered_set<sourcemeta::core::WeakPointer,
                     sourcemeta::core::WeakPointer::Hasher,
                     sourcemeta::core::WeakPointer::Comparator>
      visited;
  Subschemas subschemas;
  frame.for_each_subschema(
      [&subschemas](const sourcemeta::core::SchemaFrame::Location &location)
          -> void { subschemas.emplace_back(location); });
  const auto reached{
      arrivals(frame, edge_kinds(frame, walker, resolver, subschemas))};
  CodegenIRResult result;
  frame.for_each_subschema(
      [&](const sourcemeta::core::SchemaFrame::Location &location) -> void {
        // Framing may report resource twice or more given default identifiers
        // and nested resources
        const auto [visited_iterator, inserted] =
            visited.insert(location.pointer);
        if (!inserted) {
          return;
        }

        // Skip subschemas under validation-only keywords that do not contribute
        // to the type structure (like `contains`)
        if (is_validation_subschema(frame, location, walker, resolver)) {
          return;
        }

        // A dialect that lifts its subschemas leaves none of them under the
        // keyword that put them there, so what a reference reaches without
        // going through such a keyword is what carries a type. A subschema no
        // reference names at all is left alone, as it was before any lifting
        const auto arrival{reached.find(location.pointer)};
        if (arrival != reached.cend() && !arrival->second) {
          return;
        }

        const auto &subschema{sourcemeta::core::get(schema, location.pointer)};
        result.push_back(
            compiler(schema, frame, location, resolver, subschema));
      });

  // --------------------------------------------------------------------------
  // (5) Sort entries so that dependencies come before dependents
  // --------------------------------------------------------------------------

  std::ranges::sort(
      result,
      [](const CodegenIREntity &left, const CodegenIREntity &right) -> bool {
        return std::visit(
                   [](const auto &entry) -> auto { return entry.pointer; },
                   right) <
               std::visit(
                   [](const auto &entry) -> auto { return entry.pointer; },
                   left);
      });

  return result;
}

} // namespace sourcemeta::blaze
