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
    this->mask.erase(evaluate_path);
  }

  if (result) {
    return;
  }

  // For contains, when a subschema fails for a specific instance location,
  // we need to drop all annotations for that instance location that were
  // created within the contains subschema. This must happen BEFORE the
  // early return for masked contains failures.
  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    // Check if we're in a contains context and find the contains path
    sourcemeta::core::WeakPointer contains_path;
    bool in_contains_context = false;
    for (const auto &entry : this->mask) {
      if (evaluate_path.starts_with(entry.first) && !entry.second) {
        in_contains_context = true;
        contains_path = entry.first;
        break;
      }
    }

    if (in_contains_context) {
      // Drop annotations for this specific instance location within the
      // contains subschema
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        // Check if annotation's evaluate path starts with contains path
        if (iterator->first.evaluate_path.starts_with_initial(contains_path)) {
          // Check if instance locations match exactly
          if (iterator->first.instance_location.size() ==
              instance_location.size()) {
            bool locations_match = true;
            for (std::size_t i = 0; i < instance_location.size(); i++) {
              if (!(iterator->first.instance_location.at(i) ==
                    instance_location.at(i))) {
                locations_match = false;
                break;
              }
            }
            if (locations_match) {
              iterator = this->annotations_.erase(iterator);
              continue;
            }
          }
        }
        iterator++;
      }
    } else {
      // For non-contains contexts, drop annotations based on evaluate path
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        if (iterator->first.evaluate_path.starts_with_initial(evaluate_path)) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    }
  }

  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path](const auto &entry) {
                    return evaluate_path.starts_with(entry.first) &&
                           !entry.second;
                  })) {
    return;
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
