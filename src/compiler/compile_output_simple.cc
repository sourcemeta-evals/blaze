#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iostream>  // std::cout
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
    this->mask.erase(evaluate_path);
  }

  if (result) {
    return;
  }

  // Check if we should skip annotation dropping due to masking
  // Special handling for contains: we still want to drop annotations for
  // specific instance locations that fail, even though contains has a false
  // mask
  bool skip_due_to_mask = false;
  bool is_contains_failure = false;

  for (const auto &entry : this->mask) {
    if (evaluate_path.starts_with(entry.first) && !entry.second) {
      // Check if this is a contains subschema
      for (std::size_t i = 0; i < entry.first.size(); i++) {
        if (entry.first.at(i).is_property() &&
            entry.first.at(i).to_property() == "contains") {
          is_contains_failure = true;
          break;
        }
      }

      if (!is_contains_failure) {
        skip_due_to_mask = true;
        break;
      }
    }
  }

  if (skip_due_to_mask) {
    return;
  }

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      if (iterator->first.evaluate_path.starts_with_initial(evaluate_path)) {
        // Check if the annotation being considered for dropping is from a
        // contains subschema
        bool annotation_is_from_contains = false;
        for (std::size_t i = 0; i < iterator->first.evaluate_path.size(); i++) {
          if (iterator->first.evaluate_path.at(i).is_property() &&
              iterator->first.evaluate_path.at(i).to_property() == "contains") {
            annotation_is_from_contains = true;
            break;
          }
        }

        if (annotation_is_from_contains) {
          // For annotations from contains subschemas, only drop them if they
          // are at the specific instance_location that failed (the current
          // instance_location)
          if (iterator->first.instance_location == instance_location) {
            iterator = this->annotations_.erase(iterator);
          } else {
            iterator++;
          }
        } else {
          // For non-contains annotations, use original logic (drop all
          // matching)
          iterator = this->annotations_.erase(iterator);
        }
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
