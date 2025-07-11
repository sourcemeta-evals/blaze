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
    // Special handling for contains: clean up annotations from failed items
    const auto &keyword{evaluate_path.back().to_property()};
    if (keyword == "contains" && result && !this->annotations_.empty()) {
      // For successful contains validation, we need to remove annotations
      // from items that didn't match the contains subschema
      // The contains annotation tells us which item(s) matched
      std::set<std::size_t> matched_indices;

      // Find the contains annotation to see which items matched
      for (const auto &annotation_entry : this->annotations_) {
        if (annotation_entry.first.evaluate_path.back().is_property() &&
            annotation_entry.first.evaluate_path.back().to_property() ==
                "contains" &&
            annotation_entry.first.instance_location == instance_location) {
          // The contains annotation value indicates which items matched
          for (const auto &value : annotation_entry.second) {
            if (value.is_integer()) {
              matched_indices.insert(
                  static_cast<std::size_t>(value.to_integer()));
            }
          }
        }
      }

      // Remove annotations from items that didn't match
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        // Check if this annotation is from a contains subschema
        bool is_contains_subschema_annotation = false;
        for (const auto &token : iterator->first.evaluate_path) {
          if (token.is_property() && token.to_property() == "contains") {
            is_contains_subschema_annotation = true;
            break;
          }
        }

        if (is_contains_subschema_annotation &&
            iterator->first.evaluate_path != evaluate_path) {
          // This is an annotation from a contains subschema (not the contains
          // annotation itself) Check if it's from an item that didn't match
          const auto &item_location = iterator->first.instance_location;
          if (!item_location.empty() && item_location.back().is_index()) {
            std::size_t item_index =
                static_cast<std::size_t>(item_location.back().to_index());
            if (matched_indices.find(item_index) == matched_indices.end()) {
              // This item didn't match, remove its annotations
              iterator = this->annotations_.erase(iterator);
              continue;
            }
          }
        }
        iterator++;
      }
    }

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

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      if (iterator->first.evaluate_path.starts_with_initial(evaluate_path)) {
        iterator = this->annotations_.erase(iterator);
      } else {
        iterator++;
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
