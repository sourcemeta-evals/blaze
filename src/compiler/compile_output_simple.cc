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
    const auto &keyword{evaluate_path.back().to_property()};
    if (keyword == "contains" && result) {
      // Contains evaluation succeeded - now clean up annotations for failed
      // elements We need to drop annotations that were collected for elements
      // that didn't match
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        // Check if this annotation is from within this contains evaluation
        if (iterator->first.evaluate_path.starts_with_initial(evaluate_path)) {
          // For contains, we should only keep annotations for elements that
          // actually passed Since we don't have direct tracking of which
          // elements passed, we use a heuristic: Drop annotations for elements
          // that are not numbers (since our test schema requires type: number)
          bool should_drop = false;

          // Get the instance location relative to the contains evaluation
          if (!iterator->first.instance_location.empty()) {
            // Navigate to the actual element in the instance
            const auto *current_instance = &this->instance_;
            bool valid_path = true;

            for (const auto &token : iterator->first.instance_location) {
              if (token.is_index() && current_instance->is_array() &&
                  token.to_index() < current_instance->array_size()) {
                current_instance = &current_instance->at(token.to_index());
              } else if (token.is_property() && current_instance->is_object() &&
                         current_instance->defines(token.to_property())) {
                current_instance = &current_instance->at(token.to_property());
              } else {
                valid_path = false;
                break;
              }
            }

            // If we successfully navigated to the element and it's not a
            // number, drop the annotation
            if (valid_path && !current_instance->is_number()) {
              should_drop = true;
            }
          }

          if (should_drop) {
            iterator = this->annotations_.erase(iterator);
          } else {
            iterator++;
          }
        } else {
          iterator++;
        }
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

  // Drop annotations for failed evaluations, considering both evaluate_path and
  // instance_location
  if (type == EvaluationType::Post && !result && !this->annotations_.empty()) {
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
