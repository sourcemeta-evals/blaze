#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <set>       // std::set
#include <utility>   // std::move

namespace sourcemeta::blaze {

SimpleOutput::SimpleOutput(const sourcemeta::core::JSON &instance,
                           const sourcemeta::core::WeakPointer &base)
    : instance_{instance}, base_{base} {}

auto SimpleOutput::begin() const -> const_iterator {
  return this->output.begin();
}

auto SimpleOutput::end() const -> const_iterator { return this->output.end(); }

auto SimpleOutput::cbegin() const -> const_iterator {
  return this->output.cbegin();
}

auto SimpleOutput::cend() const -> const_iterator {
  return this->output.cend();
}

auto SimpleOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &annotation) -> void {
  if (evaluate_path.empty()) {
    return;
  }

  assert(evaluate_path.back().is_property());
  auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};
  if (effective_evaluate_path.empty()) {
    return;
  }

  if (is_annotation(step.type)) {
    if (type == EvaluationType::Post) {
      Location location{instance_location, std::move(effective_evaluate_path),
                        step.keyword_location};
      const auto match{this->annotations_.find(location)};
      if (match == this->annotations_.cend()) {
        this->annotations_[std::move(location)].push_back(annotation);

        // To avoid emitting the exact same annotation more than once
        // This is right now mostly because of `unevaluatedItems`
      } else if (match->second.back() != annotation) {
        match->second.push_back(annotation);
      }
    }

    return;
  }

  if (type == EvaluationType::Pre) {
    assert(result);
    const auto &keyword{evaluate_path.back().to_property()};
    // To ease the output
    if (keyword == "anyOf" || keyword == "oneOf" || keyword == "not" ||
        keyword == "if") {
      this->mask.emplace(evaluate_path, true);
    } else if (keyword == "contains") {
      this->mask.emplace(evaluate_path, false);
    }
  } else if (type == EvaluationType::Post &&
             this->mask.contains(evaluate_path)) {
    this->mask.erase(evaluate_path);
  }

  if (result) {
    return;
  }

  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path](const auto &entry) {
                    return evaluate_path.starts_with(entry.first) &&
                           !entry.second;
                  })) {
    return;
  }

  // Special cleanup for contains: remove title annotations for items that
  // didn't match This runs at the end of evaluation when we're back at the root
  // level
  if (type == EvaluationType::Post && result && evaluate_path.empty() &&
      instance_location.empty() && !this->annotations_.empty()) {

    // Collect all matched indices from contains annotations
    std::set<std::int64_t> matched_indices;
    for (const auto &[loc, annotations] : this->annotations_) {
      if (loc.evaluate_path.size() >= 1 &&
          loc.evaluate_path.back().is_property() &&
          loc.evaluate_path.back().to_property() == "contains" &&
          loc.instance_location.empty()) {
        for (const auto &contains_annotation : annotations) {
          if (contains_annotation.is_integer()) {
            matched_indices.insert(contains_annotation.to_integer());
          }
        }
      }
    }

    // Remove title annotations for items that didn't match
    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      if (iterator->first.evaluate_path.size() >= 2 &&
          iterator->first.evaluate_path
              .at(iterator->first.evaluate_path.size() - 2)
              .is_property() &&
          iterator->first.evaluate_path
                  .at(iterator->first.evaluate_path.size() - 2)
                  .to_property() == "contains" &&
          iterator->first.evaluate_path.back().is_property() &&
          iterator->first.evaluate_path.back().to_property() == "title") {

        if (!iterator->first.instance_location.empty() &&
            iterator->first.instance_location.back().is_index()) {
          const auto item_index =
              iterator->first.instance_location.back().to_index();

          if (matched_indices.find(static_cast<std::int64_t>(item_index)) ==
              matched_indices.end()) {
            iterator = this->annotations_.erase(iterator);
            continue;
          }
        }
      }
      iterator++;
    }
  }

  if (type == EvaluationType::Post && !this->annotations_.empty()) {

    if (!result) {
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        // For contains scenarios, we need to clean up annotations that share
        // the same instance location and have evaluate paths that are siblings
        // under the same parent
        bool should_remove = false;

        // Check if both paths are under the same parent (e.g., both under
        // /contains)
        if (iterator->first.instance_location == instance_location) {
          // Check if they share the same parent path by comparing all tokens
          // except the last
          if (!evaluate_path.empty() &&
              !iterator->first.evaluate_path.empty()) {
            // Check if they share the same parent (e.g., both under /contains)
            if (evaluate_path.size() >= 2 &&
                iterator->first.evaluate_path.size() >= 2) {
              bool same_parent = true;
              const auto min_size =
                  std::min(evaluate_path.size() - 1,
                           iterator->first.evaluate_path.size() - 1);
              for (std::size_t i = 0; i < min_size; ++i) {
                if (evaluate_path.at(i) !=
                    iterator->first.evaluate_path.at(i)) {
                  same_parent = false;
                  break;
                }
              }
              should_remove = same_parent;
            }
          }

          // Fallback to original logic for non-contains scenarios
          if (!should_remove) {
            should_remove = iterator->first.evaluate_path.starts_with_initial(
                evaluate_path);
          }
        }

        if (should_remove) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    }
  }

  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path](const auto &entry) {
                    return evaluate_path.starts_with(entry.first);
                  })) {
    return;
  }

  this->output.push_back(
      {describe(result, step, evaluate_path, instance_location, this->instance_,
                annotation),
       instance_location, std::move(effective_evaluate_path)});
}

auto SimpleOutput::stacktrace(std::ostream &stream,
                              const std::string &indentation) const -> void {
  for (const auto &entry : *this) {
    stream << indentation << entry.message << "\n";
    stream << indentation << "  at instance location \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"\n";
    stream << indentation << "  at evaluate path \"";
    sourcemeta::core::stringify(entry.evaluate_path, stream);
    stream << "\"\n";
  }
}

auto SimpleOutput::cleanup_contains_annotations() -> void {
  // Collect all matched indices from contains annotations
  std::set<std::int64_t> matched_indices;
  for (const auto &[loc, annotations] : this->annotations_) {
    if (loc.evaluate_path.size() >= 1 &&
        loc.evaluate_path.back().is_property() &&
        loc.evaluate_path.back().to_property() == "contains" &&
        loc.instance_location.empty()) {
      for (const auto &contains_annotation : annotations) {
        if (contains_annotation.is_integer()) {
          matched_indices.insert(contains_annotation.to_integer());
        }
      }
    }
  }

  // Remove title annotations for items that didn't match
  for (auto iterator = this->annotations_.begin();
       iterator != this->annotations_.end();) {
    if (iterator->first.evaluate_path.size() >= 2 &&
        iterator->first.evaluate_path
            .at(iterator->first.evaluate_path.size() - 2)
            .is_property() &&
        iterator->first.evaluate_path
                .at(iterator->first.evaluate_path.size() - 2)
                .to_property() == "contains" &&
        iterator->first.evaluate_path.back().is_property() &&
        iterator->first.evaluate_path.back().to_property() == "title") {

      if (!iterator->first.instance_location.empty() &&
          iterator->first.instance_location.back().is_index()) {
        const auto item_index =
            iterator->first.instance_location.back().to_index();

        if (matched_indices.find(static_cast<std::int64_t>(item_index)) ==
            matched_indices.end()) {
          iterator = this->annotations_.erase(iterator);
          continue;
        }
      }
    }
    iterator++;
  }
}

} // namespace sourcemeta::blaze
