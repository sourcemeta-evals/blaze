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
      this->mask.emplace(std::make_pair(evaluate_path, instance_location),
                         true);
    } else if (keyword == "contains") {
      this->mask.emplace(std::make_pair(evaluate_path, instance_location),
                         false);
    }
  } else if (type == EvaluationType::Post) {
    const auto mask_key{std::make_pair(evaluate_path, instance_location)};
    if (this->mask.contains(mask_key)) {
      this->mask.erase(mask_key);
    }
  }

  if (result) {
    return;
  }

  // Clean up annotations for failed instructions BEFORE early return
  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    // Find the nearest masked ancestor (e.g., contains) for this failure
    auto effective_evaluate_path_for_cleanup{
        evaluate_path.resolve_from(this->base_)};
    sourcemeta::core::WeakPointer cleanup_prefix{
        effective_evaluate_path_for_cleanup};
    bool found_contains_mask = false;

    for (const auto &entry : this->mask) {
      if (evaluate_path.starts_with(entry.first.first) &&
          instance_location.starts_with(entry.first.second) && !entry.second) {
        // Use the contains ancestor path as cleanup prefix
        auto resolved_mask_path{entry.first.first.resolve_from(this->base_)};
        cleanup_prefix = resolved_mask_path;
        found_contains_mask = true;
        break;
      }
    }

    // Remove annotations under the cleanup prefix for this instance location
    // For contains, we need to remove annotations that were added for this
    // specific instance location under the contains evaluate path
    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      bool should_remove = false;
      
      if (found_contains_mask) {
        // For contains failures, remove annotations under the contains path
        // for this exact instance location
        should_remove = iterator->first.evaluate_path.starts_with(cleanup_prefix) &&
                       iterator->first.instance_location == instance_location;
      } else {
        // For other failures, use the original logic
        should_remove = iterator->first.evaluate_path.starts_with_initial(
                           effective_evaluate_path_for_cleanup) &&
                       iterator->first.instance_location.starts_with(instance_location);
      }
      
      if (should_remove) {
        iterator = this->annotations_.erase(iterator);
      } else {
        iterator++;
      }
    }
  }

  // Early return for nested failures under contains (suppress error output)
  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path, &instance_location](const auto &entry) {
                    return evaluate_path.starts_with(entry.first.first) &&
                           instance_location.starts_with(entry.first.second) &&
                           !entry.second;
                  })) {
    return;
  }

  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path, &instance_location](const auto &entry) {
                    return evaluate_path.starts_with(entry.first.first) &&
                           instance_location.starts_with(entry.first.second);
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
