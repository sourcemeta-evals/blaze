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

  // Handle contains-specific annotation cleanup for failed items
  // This must happen before the early return for successful results
  if (type == EvaluationType::Post && !evaluate_path.empty() && result) {
    const auto &keyword{evaluate_path.back().to_property()};

    // Check if we just finished evaluating a contains instruction successfully
    if (keyword == "contains") {

      // The contains succeeded overall, but we need to remove annotations
      // from items that failed the subschema validation
      // We'll track which items should have annotations by checking
      // if they have the contains annotation itself

      std::set<sourcemeta::core::WeakPointer> successful_items;

      // First pass: find all items that have the contains annotation
      // (these are the ones that matched)
      for (const auto &entry : this->annotations_) {
        if (entry.first.evaluate_path == effective_evaluate_path) {
          // This is the contains annotation itself, the value tells us which
          // items matched
          for (const auto &annotation_value : entry.second) {
            if (annotation_value.is_integer()) {
              sourcemeta::core::WeakPointer successful_item_location =
                  instance_location;
              successful_item_location.push_back(annotation_value.to_integer());
              successful_items.insert(successful_item_location);
            }
          }
        }
      }

      // Second pass: remove annotations from items that are NOT in the
      // successful set
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        // Check if this annotation is for a contains subschema
        if (iterator->first.evaluate_path.starts_with_initial(
                effective_evaluate_path) &&
            iterator->first.evaluate_path != effective_evaluate_path) {

          // Check if this annotation's instance location is for a successful
          // item
          bool is_successful_item = false;
          for (const auto &successful_location : successful_items) {
            if (iterator->first.instance_location == successful_location) {
              is_successful_item = true;
              break;
            }
          }

          if (!is_successful_item) {
            iterator = this->annotations_.erase(iterator);
          } else {
            iterator++;
          }
        } else {
          iterator++;
        }
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
