#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <optional>  // std::optional
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
    // Special handling for contains: clean up annotations from failed
    // validations
    if (evaluate_path.back().is_property() &&
        evaluate_path.back().to_property() == "contains" &&
        !this->annotations_.empty()) {

      // For contains, we need to clean up annotations from items that didn't
      // match We do this by checking if there's a successful contains
      // annotation
      bool contains_succeeded = false;
      std::optional<sourcemeta::core::JSON> contains_annotation_value;

      // Find the contains annotation to see which item(s) matched
      for (const auto &[location, entries] : this->annotations_) {
        if (location.evaluate_path.back().is_property() &&
            location.evaluate_path.back().to_property() == "contains" &&
            location.instance_location.empty()) {
          contains_succeeded = true;
          if (!entries.empty()) {
            contains_annotation_value = entries.back();
          }
          break;
        }
      }

      if (contains_succeeded && contains_annotation_value &&
          contains_annotation_value->is_integer()) {
        // Clean up annotations from non-matching items
        const auto matching_index = contains_annotation_value->to_integer();
        for (auto iterator = this->annotations_.begin();
             iterator != this->annotations_.end();) {
          // Check if this annotation is from a contains subschema
          if (iterator->first.evaluate_path.starts_with_initial(
                  evaluate_path) &&
              !iterator->first.instance_location.empty()) {

            // Extract the array index from the instance location
            const auto &instance_loc = iterator->first.instance_location;
            if (instance_loc.size() == 1 && instance_loc.back().is_index()) {
              const auto item_index =
                  static_cast<std::int64_t>(instance_loc.back().to_index());
              // Remove annotation if it's not from the matching item
              if (item_index != matching_index) {
                iterator = this->annotations_.erase(iterator);
                continue;
              }
            }
          }
          iterator++;
        }
      }
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

  if (type == EvaluationType::Post && !result && !this->annotations_.empty()) {
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
