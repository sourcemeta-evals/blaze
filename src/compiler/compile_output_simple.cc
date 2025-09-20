#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
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
    // Special handling for contains - clean up annotations from non-matching
    // items
    if (result && evaluate_path.back().is_property() &&
        evaluate_path.back().to_property() == "contains") {
      this->cleanup_contains_annotations(evaluate_path, annotation);
    }
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

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
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

auto SimpleOutput::cleanup_contains_annotations(
    const sourcemeta::core::WeakPointer &contains_evaluate_path,
    const sourcemeta::core::JSON &contains_annotation) -> void {
  // Find the actual contains annotation in our annotations collection
  // The contains annotation should be at instance location "" and evaluate path
  // "/contains"
  std::set<std::size_t> matched_indices;

  // Look for the contains annotation
  for (const auto &annotation_entry : this->annotations_) {
    if (annotation_entry.first.instance_location.empty() &&
        annotation_entry.first.evaluate_path.back().is_property() &&
        annotation_entry.first.evaluate_path.back().to_property() ==
            "contains") {

      // The contains annotation can be either an array of integers or a single
      // integer
      for (const auto &annotation_value : annotation_entry.second) {
        if (annotation_value.is_array()) {
          for (const auto &item : annotation_value.as_array()) {
            if (item.is_integer()) {
              const auto index = static_cast<std::size_t>(item.to_integer());
              matched_indices.insert(index);
            }
          }
          break; // Found the array, no need to continue
        } else if (annotation_value.is_integer()) {
          const auto index =
              static_cast<std::size_t>(annotation_value.to_integer());
          matched_indices.insert(index);
        }
      }
      break; // Found the contains annotation, no need to continue
    }
  }

  if (matched_indices.empty()) {
    return;
  }

  // Remove annotations for all items that didn't match
  for (auto iterator = this->annotations_.begin();
       iterator != this->annotations_.end();) {
    // Check if this annotation is under the contains path
    if (iterator->first.evaluate_path.size() > contains_evaluate_path.size() &&
        iterator->first.evaluate_path.starts_with_initial(
            contains_evaluate_path)) {

      // Extract the index from the instance location
      if (!iterator->first.instance_location.empty() &&
          iterator->first.instance_location.back().is_index()) {
        const auto item_index =
            iterator->first.instance_location.back().to_index();

        // Remove annotation if this item didn't match
        if (matched_indices.find(item_index) == matched_indices.end()) {
          iterator = this->annotations_.erase(iterator);
          continue;
        }
      }
    }
    iterator++;
  }
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
