#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
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
        this->annotations_[std::move(location)].push_back(annotation);
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
    // When contains finishes, clean up annotations for items that didn't match
    // The contains annotation tells us which items matched
    const auto &keyword{evaluate_path.back().to_property()};
    if (keyword == "contains" && result) {
      // Find the contains annotation to see which items matched
      std::set<std::size_t> matched_indices;
      for (const auto &ann_entry : this->annotations_) {
        if (ann_entry.first.evaluate_path == evaluate_path &&
            ann_entry.first.instance_location.empty()) {
          // This is the contains annotation at the root level
          for (const auto &value : ann_entry.second) {
            if (value.is_integer()) {
              matched_indices.insert(
                  static_cast<std::size_t>(value.to_integer()));
            }
          }
        }
      }

      // Remove annotations for items that didn't match
      for (auto it = this->annotations_.begin();
           it != this->annotations_.end();) {
        if (it->first.evaluate_path.starts_with_initial(evaluate_path) &&
            !it->first.instance_location.empty()) {
          // This is an annotation under the contains path at a specific item
          // location Check if this item matched
          if (!it->first.instance_location.empty() &&
              it->first.instance_location.back().is_index()) {
            std::size_t item_index =
                it->first.instance_location.back().to_index();
            if (matched_indices.find(item_index) == matched_indices.end()) {
              // This item didn't match, remove its annotations
              it = this->annotations_.erase(it);
              continue;
            }
          }
        }
        ++it;
      }
    }

    this->mask.erase(evaluate_path);

    // Clean up contains_failed_ entries for this contains block
    for (auto it = this->contains_failed_.begin();
         it != this->contains_failed_.end();) {
      if (it->first == evaluate_path) {
        it = this->contains_failed_.erase(it);
      } else {
        ++it;
      }
    }
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

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    // Check if we're inside a 'contains' keyword by looking at the mask
    bool inside_contains = false;
    sourcemeta::core::WeakPointer contains_path;
    for (const auto &mask_entry : this->mask) {
      if (!mask_entry.second && evaluate_path.starts_with(mask_entry.first)) {
        inside_contains = true;
        contains_path = mask_entry.first;
        break;
      }
    }

    if (inside_contains) {
      // Record this as a failed contains item
      this->contains_failed_.emplace(contains_path, instance_location);

      // Clean up any annotations already collected for this item
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        if (iterator->first.evaluate_path.starts_with_initial(contains_path) &&
            iterator->first.instance_location.starts_with_initial(
                instance_location)) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    } else {
      // For other keywords, use the original logic
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        if (iterator->first.evaluate_path.starts_with_initial(evaluate_path) &&
            iterator->first.instance_location.starts_with_initial(
                instance_location)) {
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

} // namespace sourcemeta::blaze
