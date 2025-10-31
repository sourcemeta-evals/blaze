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
    // When contains finishes, drop annotations for failed items
    const auto &keyword{evaluate_path.back().to_property()};
    if (keyword == "contains" && !this->annotations_.empty()) {
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        // Check if this annotation is under the contains evaluate path
        if (!iterator->first.evaluate_path.starts_with_initial(evaluate_path)) {
          iterator++;
          continue;
        }

        // Check if this annotation's instance location had a failure
        bool should_drop = false;
        for (const auto &[failure_eval_path, failure_inst_loc] :
             this->contains_failures) {
          if (failure_eval_path.starts_with(evaluate_path) &&
              iterator->first.instance_location.starts_with(failure_inst_loc)) {
            should_drop = true;
            break;
          }
        }

        if (should_drop) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }

      // Clean up failure tracking for this contains
      for (auto it = this->contains_failures.begin();
           it != this->contains_failures.end();) {
        if (it->first.starts_with(evaluate_path)) {
          it = this->contains_failures.erase(it);
        } else {
          ++it;
        }
      }
    }

    this->mask.erase(evaluate_path);
  }

  if (result) {
    return;
  }

  // Check if we're in a contains context (mask entry with false value)
  const bool in_contains_context = std::any_of(
      this->mask.cbegin(), this->mask.cend(),
      [&evaluate_path](const auto &entry) {
        return evaluate_path.starts_with(entry.first) && !entry.second;
      });

  if (in_contains_context) {
    // Track this failure for contains so we can drop annotations later
    this->contains_failures.emplace(evaluate_path, instance_location);
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
