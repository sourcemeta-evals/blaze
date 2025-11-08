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
      this->mask.emplace(MaskLocation{evaluate_path, instance_location}, true);
    } else if (keyword == "contains") {
      this->mask.emplace(MaskLocation{evaluate_path, instance_location}, false);
    }
  } else if (type == EvaluationType::Post) {
    const MaskLocation mask_key{evaluate_path, instance_location};
    if (this->mask.contains(mask_key)) {
      this->mask.erase(mask_key);
    }
  }

  if (result) {
    return;
  }

  // Clean up annotations on failure BEFORE suppressing masked errors
  // Only run cleanup when under a 'contains' mask to avoid wiping unrelated
  // annotations
  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    // Check if we're under a contains mask
    bool under_contains_mask = false;
    for (const auto &mask_entry : this->mask) {
      const bool instance_in_mask =
          mask_entry.first.instance_location.empty() ||
          instance_location.starts_with(mask_entry.first.instance_location);
      if (!mask_entry.second &&
          evaluate_path.starts_with(mask_entry.first.evaluate_path) &&
          instance_in_mask) {
        under_contains_mask = true;
        break;
      }
    }

    if (under_contains_mask) {
      // Only run masked cleanup for failed items in contains
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        bool should_erase = false;

        for (const auto &mask_entry : this->mask) {
          const bool instance_in_mask =
              mask_entry.first.instance_location.empty() ||
              instance_location.starts_with(mask_entry.first.instance_location);
          if (!mask_entry.second &&
              evaluate_path.starts_with(mask_entry.first.evaluate_path) &&
              instance_in_mask &&
              iterator->first.evaluate_path.starts_with(
                  mask_entry.first.evaluate_path) &&
              iterator->first.instance_location == instance_location) {
            should_erase = true;
            break;
          }
        }

        if (should_erase) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    }
  }

  // Standard cleanup for non-masked failures
  if (type == EvaluationType::Post && !this->annotations_.empty() &&
      !std::any_of(
          this->mask.cbegin(), this->mask.cend(),
          [&evaluate_path, &instance_location](const auto &entry) {
            const bool instance_in_mask =
                entry.first.instance_location.empty() ||
                instance_location.starts_with(entry.first.instance_location);
            return !entry.second &&
                   evaluate_path.starts_with(entry.first.evaluate_path) &&
                   instance_in_mask;
          })) {
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

  // Suppress errors for masked contexts (like contains with false value)
  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path, &instance_location](const auto &entry) {
                    return evaluate_path.starts_with(
                               entry.first.evaluate_path) &&
                           instance_location.starts_with(
                               entry.first.instance_location) &&
                           !entry.second;
                  })) {
    return;
  }

  // Suppress all output for masked contexts
  if (std::any_of(
          this->mask.cbegin(), this->mask.cend(),
          [&evaluate_path, &instance_location](const auto &entry) {
            return evaluate_path.starts_with(entry.first.evaluate_path) &&
                   instance_location.starts_with(entry.first.instance_location);
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
