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
        this->filtered_annotations_dirty_ = true;

        // To avoid emitting the exact same annotation more than once
        // This is right now mostly because of `unevaluatedItems`
      } else if (match->second.back() != annotation) {
        match->second.push_back(annotation);
        this->filtered_annotations_dirty_ = true;
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

auto SimpleOutput::filtered_annotations() const
    -> const std::map<Location, std::vector<sourcemeta::core::JSON>> & {
  if (!this->filtered_annotations_dirty_) {
    return this->filtered_annotations_cache_;
  }

  this->filtered_annotations_cache_.clear();

  // First, collect all contains annotations to find successful indices
  std::map<sourcemeta::core::WeakPointer, std::set<std::int64_t>>
      contains_successful_items;

  for (const auto &entry : this->annotations_) {
    const auto &location = entry.first;
    const auto &values = entry.second;

    // Check if this is a contains annotation
    if (!location.evaluate_path.empty() &&
        location.evaluate_path.back().to_property() == "contains" &&
        location.instance_location.empty()) {
      // This is a contains annotation at the root level
      for (const auto &value : values) {
        if (value.is_integer()) {
          const auto array_location = location.instance_location;
          contains_successful_items[array_location].insert(value.to_integer());
        }
      }
    }
  }

  // Now filter annotations based on contains success
  for (const auto &entry : this->annotations_) {
    const auto &location = entry.first;
    const auto &values = entry.second;
    bool should_include = true;

    // Check if this annotation is for a contains subschema
    if (location.evaluate_path.size() >= 2) {
      const auto &eval_path = location.evaluate_path;

      // Check if this is a subschema annotation under contains
      for (std::size_t i = 0; i < eval_path.size() - 1; i++) {
        if (eval_path.at(i).is_property() &&
            eval_path.at(i).to_property() == "contains") {
          // This is an annotation from a contains subschema
          // Check if the instance location corresponds to a successful item

          // Find the array location (parent of the item)
          sourcemeta::core::WeakPointer array_location;
          std::int64_t item_index = -1;

          if (!location.instance_location.empty()) {
            array_location = location.instance_location.initial();
            const auto &last_token = location.instance_location.back();
            if (last_token.is_index()) {
              item_index = static_cast<std::int64_t>(last_token.to_index());
            }
          }

          // Check if this item was successful in contains validation
          const auto successful_items_it =
              contains_successful_items.find(array_location);
          if (successful_items_it != contains_successful_items.end()) {
            const auto &successful_indices = successful_items_it->second;
            if (item_index >= 0 && successful_indices.find(item_index) ==
                                       successful_indices.end()) {
              // This item was not successful, exclude the annotation
              should_include = false;
            }
          }
          break;
        }
      }
    }

    if (should_include) {
      this->filtered_annotations_cache_[location] = values;
    }
  }

  this->filtered_annotations_dirty_ = false;
  return this->filtered_annotations_cache_;
}

} // namespace sourcemeta::blaze
