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
    this->mask.erase({evaluate_path, instance_location});
  }

  if (result) {
    return;
  }

  // Drop annotations for failed validations
  // For masked parents (like contains), we need to check both evaluate path
  // and instance location to properly drop annotations for failed items
  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    // Check if this failure is under a masked parent (e.g., contains)
    sourcemeta::core::WeakPointer masked_parent_evaluate_path;
    sourcemeta::core::WeakPointer masked_parent_instance_location;
    bool has_masked_parent = false;
    
    for (const auto &entry : this->mask) {
      if (evaluate_path.starts_with(entry.first)) {
        masked_parent_evaluate_path = entry.first;
        masked_parent_instance_location = entry.second;
        has_masked_parent = true;
        break;
      }
    }

    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      bool should_drop = false;

      if (has_masked_parent) {
        // For masked parents (like contains), drop annotations that are:
        // 1. STRICTLY under the masked parent evaluate path (not equal to it)
        // 2. At a child instance location of the masked parent (for contains,
        //    the parent is at root "", children are at /0, /1, /2, etc.)
        // 3. At the current failing instance location
        // This ensures we don't drop the annotation emitted by the masked
        // parent itself, and only drop annotations for the specific failing item
        should_drop =
            iterator->first.evaluate_path.starts_with(masked_parent_evaluate_path) &&
            iterator->first.evaluate_path.size() > masked_parent_evaluate_path.size() &&
            iterator->first.instance_location.starts_with_initial(masked_parent_instance_location) &&
            iterator->first.instance_location == instance_location;
      } else {
        // For non-masked failures, drop annotations that are:
        // 1. Under the same or descendant evaluate path
        // 2. At the exact same instance location
        should_drop =
            iterator->first.evaluate_path.starts_with_initial(evaluate_path) &&
            iterator->first.instance_location == instance_location;
      }

      if (should_drop) {
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
