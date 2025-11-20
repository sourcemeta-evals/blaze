#include <sourcemeta/blaze/output_simple.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <utility>   // std::move

namespace sourcemeta::blaze {

SimpleOutput::SimpleOutput(const sourcemeta::core::JSON &instance,
                           sourcemeta::core::WeakPointer base)
    : instance_{instance}, base_{std::move(base)} {}

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
      Location location{.instance_location = instance_location,
                        .evaluate_path = std::move(effective_evaluate_path),
                        .schema_location = step.keyword_location};
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
        keyword == "if" || keyword == "contains") {
      this->mask.emplace(evaluate_path, instance_location);
      // For contains, initialize failure tracking
      if (keyword == "contains") {
        this->mask_failures.emplace(evaluate_path,
                                    std::set<sourcemeta::core::WeakPointer>{});
      }
    }
  } else if (type == EvaluationType::Post &&
             this->mask.contains({evaluate_path, instance_location})) {
    // Before cleaning up, drop annotations for failed instance locations
    const auto failures_it = this->mask_failures.find(evaluate_path);
    if (failures_it != this->mask_failures.end() &&
        !failures_it->second.empty()) {
      // Drop annotations for instance locations that failed under this masked
      // path
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        // Check if this annotation is under the masked evaluate path
        // and its instance location is in the failed set
        if (iterator->first.evaluate_path.starts_with_initial(evaluate_path) &&
            failures_it->second.contains(iterator->first.instance_location)) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    }

    this->mask.erase({evaluate_path, instance_location});
    // Clean up failure tracking when the masked keyword completes
    this->mask_failures.erase(evaluate_path);
  }

  if (result) {
    return;
  }

  // Track failures under masked paths (like contains)
  // When a subschema fails under a masked path, record the instance location
  for (const auto &mask_entry : this->mask) {
    if (evaluate_path.starts_with(mask_entry.first)) {
      // This is a failure under a masked path
      // Record this instance location as failed for this masked path
      auto failures_it = this->mask_failures.find(mask_entry.first);
      if (failures_it != this->mask_failures.end()) {
        failures_it->second.insert(instance_location);
      }
    }
  }

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      if (iterator->first.evaluate_path.starts_with_initial(evaluate_path) &&
          iterator->first.instance_location == instance_location) {
        iterator = this->annotations_.erase(iterator);
      } else {
        iterator++;
      }
    }
  }

  if (std::ranges::any_of(this->mask, [&evaluate_path](const auto &entry) {
        return evaluate_path.starts_with(entry.first);
      })) {
    return;
  }

  this->output.push_back(
      {describe(result, step, evaluate_path, instance_location, this->instance_,
                annotation),
       instance_location, std::move(effective_evaluate_path),
       step.keyword_location});
}

} // namespace sourcemeta::blaze
