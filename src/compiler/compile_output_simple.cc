#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iostream>  // std::cerr
#include <iterator>  // std::back_inserter
#include <set>       // std::set
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

  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path](const auto &entry) {
                    return evaluate_path.starts_with(entry.first) &&
                           !entry.second;
                  })) {
    return;
  }

  // Record failed validation for annotation filtering
  if (type == EvaluationType::Post) {
    this->failed_validations_.emplace(instance_location, evaluate_path);
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

auto SimpleOutput::cleanup_failed_contains_annotations() -> void {
  // Only proceed if there are contains-related annotations
  bool has_contains_annotations = false;
  for (const auto &annotation : this->annotations_) {
    for (std::size_t i = 0; i < annotation.first.evaluate_path.size(); ++i) {
      if (annotation.first.evaluate_path.at(i).is_property() &&
          annotation.first.evaluate_path.at(i).to_property() == "contains") {
        has_contains_annotations = true;
        break;
      }
    }
    if (has_contains_annotations)
      break;
  }

  if (!has_contains_annotations) {
    return;
  }

  // Find the contains annotation at root level to determine which indices
  // succeeded
  std::set<int> successful_indices;

  for (const auto &annotation : this->annotations_) {
    // Look for contains annotation at root level (empty instance location)
    if (annotation.first.instance_location.empty() &&
        annotation.first.evaluate_path.size() >= 1 &&
        annotation.first.evaluate_path.back().is_property() &&
        annotation.first.evaluate_path.back().to_property() == "contains") {

      // Extract successful indices from the contains annotation values
      for (const auto &value : annotation.second) {
        if (value.is_integer()) {
          int index = static_cast<int>(value.to_integer());
          successful_indices.insert(index);
        }
      }
      break;
    }
  }

  // Remove annotations from instance locations that are not in
  // successful_indices
  for (auto iterator = this->annotations_.begin();
       iterator != this->annotations_.end();) {
    bool should_drop = false;

    // Check if this is a contains-related annotation at a specific instance
    // location
    if (!iterator->first.instance_location.empty() &&
        iterator->first.instance_location.size() == 1 &&
        iterator->first.instance_location.at(0).is_index()) {

      // Check if the evaluate path contains "contains"
      bool is_contains_annotation = false;
      for (std::size_t i = 0; i < iterator->first.evaluate_path.size(); ++i) {
        if (iterator->first.evaluate_path.at(i).is_property() &&
            iterator->first.evaluate_path.at(i).to_property() == "contains") {
          is_contains_annotation = true;
          break;
        }
      }

      if (is_contains_annotation) {
        // Get the array index from the instance location
        int array_index = static_cast<int>(
            iterator->first.instance_location.at(0).to_index());

        // Drop annotation if this index is not in the successful set
        if (successful_indices.find(array_index) == successful_indices.end()) {
          should_drop = true;
        }
      }
    }

    if (should_drop) {
      iterator = this->annotations_.erase(iterator);
    } else {
      iterator++;
    }
  }
}

} // namespace sourcemeta::blaze
