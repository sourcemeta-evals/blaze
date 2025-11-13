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
    }
  } else if (type == EvaluationType::Post &&
             this->mask.contains({evaluate_path, instance_location})) {
    const auto &keyword{evaluate_path.back().to_property()};

    // For contains, drop annotations for failed items before cleaning up
    if (keyword == "contains" && !this->annotations_.empty()) {
      for (auto it = this->annotations_.begin();
           it != this->annotations_.end();) {
        bool should_drop = false;

        // Check if this annotation is under a contains failure with matching
        // instance location
        for (const auto &failure : this->contains_failures) {
          if (failure.first == evaluate_path &&
              it->first.evaluate_path.starts_with_initial(evaluate_path) &&
              it->first.instance_location.starts_with(failure.second)) {
            should_drop = true;
            break;
          }
        }

        if (should_drop) {
          it = this->annotations_.erase(it);
        } else {
          ++it;
        }
      }

      // Clean up the failure tracking for this contains
      for (auto it = this->contains_failures.begin();
           it != this->contains_failures.end();) {
        if (it->first == evaluate_path) {
          it = this->contains_failures.erase(it);
        } else {
          ++it;
        }
      }
    }

    this->mask.erase({evaluate_path, instance_location});
  }

  if (result) {
    return;
  }

  // Track failures for contains to know which instance locations failed
  const auto contains_mask =
      std::find_if(this->mask.cbegin(), this->mask.cend(),
                   [&evaluate_path](const auto &entry) {
                     return evaluate_path.starts_with(entry.first);
                   });

  if (contains_mask != this->mask.cend()) {
    const auto &mask_keyword{contains_mask->first.back().to_property()};
    // Only track failures for contains, not for anyOf/oneOf/not/if
    if (mask_keyword == "contains") {
      // We're inside a contains evaluation that is tracking failures
      // Record this failure with both evaluate_path and instance_location
      this->contains_failures.emplace(contains_mask->first, instance_location);
      return;
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
