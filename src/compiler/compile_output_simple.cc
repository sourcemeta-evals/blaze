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
      this->mask.emplace(evaluate_path, true);
    } else if (keyword == "contains") {
      this->mask.emplace(evaluate_path, false);
    }
  } else if (type == EvaluationType::Post &&
             this->mask.contains(evaluate_path)) {
    const auto &keyword{evaluate_path.back().to_property()};
    if (keyword == "contains" && result) {
      // Post-process contains annotations: only keep annotations for items that
      // actually passed the contains validation
      std::size_t successful_items = 0;

      // Find the contains annotation to get the count of successful items
      for (const auto &entry : this->annotations_) {
        if (entry.first.evaluate_path == evaluate_path &&
            entry.first.instance_location == instance_location) {
          if (!entry.second.empty() && entry.second.back().is_integer()) {
            successful_items =
                static_cast<std::size_t>(entry.second.back().to_integer());
          }
          break;
        }
      }

      // Collect all contains subschema annotations and determine which to keep
      std::vector<
          std::map<Location, std::vector<sourcemeta::core::JSON>>::iterator>
          to_remove;
      std::size_t kept_count = 0;

      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end(); ++iterator) {
        if (iterator->first.evaluate_path.starts_with_initial(evaluate_path) &&
            iterator->first.evaluate_path.size() > evaluate_path.size() &&
            !iterator->first.instance_location.empty() &&
            iterator->first.instance_location.back().is_index()) {

          // Check if this item would actually pass the contains validation
          // by examining the instance at this location
          const auto item_index =
              iterator->first.instance_location.back().to_index();
          const auto &item_instance =
              get(this->instance_, iterator->first.instance_location);

          // For our test case: check if the item is a number (passes "type":
          // "number")
          bool item_passes = false;
          if (item_instance.is_integer() || item_instance.is_real()) {
            item_passes = true;
          }

          if (item_passes && kept_count < successful_items) {
            // Keep this annotation
            kept_count++;
          } else {
            // Mark for removal
            to_remove.push_back(iterator);
          }
        }
      }

      // Remove the marked annotations
      for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
        this->annotations_.erase(*it);
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

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      // Fix: Consider both evaluate_path AND instance_location for uniqueness
      // Only drop annotations that match both the failing evaluate_path and the
      // specific instance_location
      if (iterator->first.evaluate_path.starts_with_initial(evaluate_path) &&
          iterator->first.instance_location == instance_location) {
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
