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

  // Check if we're under a masked keyword (anyOf, oneOf, not, if, contains)
  // For contains specifically, we need to drop annotations before the early
  // return
  const auto masked_entry = std::ranges::find_if(
      this->mask, [&evaluate_path, &instance_location](const auto &entry) {
        return evaluate_path.starts_with(entry.first) &&
               entry.second == instance_location;
      });

  if (masked_entry != this->mask.cend()) {
    // Only drop annotations for contains keyword
    // For anyOf/oneOf/not/if, we want to keep annotations from successful
    // branches
    assert(!masked_entry->first.empty());
    assert(masked_entry->first.back().is_property());
    const auto &masked_keyword{masked_entry->first.back().to_property()};

    if (masked_keyword == "contains") {
      // Drop annotations for this specific instance location under the contains
      // subschema
      if (type == EvaluationType::Post && !this->annotations_.empty()) {
        const auto &masked_path = masked_entry->first;
        for (auto iterator = this->annotations_.begin();
             iterator != this->annotations_.end();) {
          if (iterator->first.instance_location == instance_location &&
              iterator->first.evaluate_path.starts_with_initial(masked_path)) {
            iterator = this->annotations_.erase(iterator);
          } else {
            iterator++;
          }
        }
      }
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
