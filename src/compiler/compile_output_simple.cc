#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iostream>  // std::cerr
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

auto SimpleOutput::cleanup_contains_annotations() -> void {
  // Find all contains annotations that track successful matches
  std::map<sourcemeta::core::WeakPointer, std::set<std::size_t>>
      successful_contains_matches;

  for (const auto &entry : this->annotations_) {
    const auto &location = entry.first;
    const auto &annotations = entry.second;

    // Look for contains annotations that track successful matches
    if (location.evaluate_path.size() >= 1 &&
        location.evaluate_path.back().is_property() &&
        location.evaluate_path.back().to_property() == "contains") {

      // This is a contains annotation - it contains the indices of successful
      // matches
      for (const auto &annotation : annotations) {

        if (annotation.is_integer()) {
          // The contains annotation is at the array level (empty or root
          // instance location) but we need to map it to the array location for
          // filtering
          const auto array_location = location.instance_location;
          const auto match_index =
              static_cast<std::size_t>(annotation.to_integer());
          successful_contains_matches[array_location].insert(match_index);
        }
      }
    }
  }

  // Remove annotations for array items that didn't match any contains subschema
  for (auto iterator = this->annotations_.begin();
       iterator != this->annotations_.end();) {
    const auto &location = iterator->first;

    // Check if this annotation is for a contains subschema
    bool is_contains_related = false;
    sourcemeta::core::WeakPointer array_location;
    std::size_t item_index = 0;

    // Check if the evaluate path indicates this is a contains-related
    // annotation, but exclude the contains annotation itself
    for (std::size_t i = 0; i < location.evaluate_path.size(); ++i) {
      if (location.evaluate_path.at(i).is_property() &&
          location.evaluate_path.at(i).to_property() == "contains") {
        // Only process annotations that are INSIDE contains (like title),
        // not the contains annotation itself
        if (i + 1 < location.evaluate_path.size()) {
          is_contains_related = true;

          // Extract the array location and item index from instance location
          if (!location.instance_location.empty() &&
              location.instance_location.back().is_index()) {
            item_index = location.instance_location.back().to_index();
            array_location = location.instance_location.initial();
          }
        }
        break;
      }
    }

    if (is_contains_related) {
      // For contains annotations, we need to check against the empty array
      // location since that's where the contains annotation is stored
      const auto empty_location = sourcemeta::core::empty_weak_pointer;
      const auto matches_it = successful_contains_matches.find(empty_location);
      if (matches_it != successful_contains_matches.end()) {
        const auto &successful_indices = matches_it->second;

        // If this item index is not in the successful matches, remove the
        // annotation
        if (successful_indices.find(item_index) == successful_indices.end()) {
          iterator = this->annotations_.erase(iterator);
          continue;
        }
      }
    }

    ++iterator;
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
