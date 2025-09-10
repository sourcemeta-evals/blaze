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
  } else if (type == EvaluationType::Post) {
    // Check if this evaluation is related to any mask entry
    bool is_masked_evaluation = this->mask.contains(evaluate_path);
    bool is_contains_subschema = false;
    sourcemeta::core::WeakPointer contains_mask_path;

    // Also check if this is a subschema evaluation within a contains context
    if (!is_masked_evaluation) {
      for (const auto &mask_entry : this->mask) {
        if (mask_entry.first.back().is_property() &&
            mask_entry.first.back().to_property() == "contains" &&
            evaluate_path.starts_with_initial(mask_entry.first)) {
          is_contains_subschema = true;
          contains_mask_path = mask_entry.first;
          break;
        }
      }
    }

    if (is_masked_evaluation || is_contains_subschema) {
      // Track successful contains item evaluations
      if (result && !instance_location.empty() && is_contains_subschema) {
        // This specific instance location passed a subschema within contains
        this->successful_contains_items.emplace(contains_mask_path,
                                                instance_location);
      }

      if (is_masked_evaluation) {
        this->mask.erase(evaluate_path);
      }
    }
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
    // Check if this is a contains subschema evaluation that failed
    bool is_contains_subschema = false;
    sourcemeta::core::WeakPointer contains_path;

    // Look for a contains keyword in the evaluate path
    for (std::size_t i = 0; i < evaluate_path.size(); i++) {
      if (evaluate_path.at(i).is_property() &&
          evaluate_path.at(i).to_property() == "contains") {
        is_contains_subschema = true;
        // Get the path up to and including the contains keyword
        contains_path = evaluate_path;
        while (contains_path.size() > i + 1) {
          contains_path.pop_back();
        }
        break;
      }
    }

    for (auto iterator = this->annotations_.begin();
         iterator != this->annotations_.end();) {
      bool should_drop = false;

      if (iterator->first.evaluate_path.starts_with_initial(evaluate_path)) {
        if (is_contains_subschema && !instance_location.empty()) {
          // For contains subschema failures, only drop annotations for the
          // specific instance location that failed
          should_drop =
              (iterator->first.instance_location == instance_location);
        } else {
          // For other failures, drop all annotations under the evaluate path
          should_drop = true;
        }
      }

      if (should_drop) {
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
