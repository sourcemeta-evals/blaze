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

      // Just collect the annotation normally - cleanup happens at the end

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
      bool should_erase = false;

      // Check if we're in a contains subschema evaluation that failed
      bool is_contains_subschema = false;
      for (std::size_t i = 0; i < evaluate_path.size(); ++i) {
        if (evaluate_path.at(i).is_property() &&
            evaluate_path.at(i).to_property() == "contains") {
          is_contains_subschema = true;
          break;
        }
      }

      if (!result && is_contains_subschema) {
        // For contains failures, clean up annotations for the specific instance
        // location
        should_erase =
            iterator->first.instance_location == instance_location &&
            iterator->first.evaluate_path.starts_with_initial(evaluate_path);
      } else {
        should_erase =
            iterator->first.evaluate_path.starts_with_initial(evaluate_path);
      }

      if (should_erase) {
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
  // Find all contains annotations to determine which array indices passed
  std::set<std::size_t> passing_indices;
  for (const auto &annotation_entry : this->annotations_) {
    if (annotation_entry.first.instance_location.empty() &&
        annotation_entry.first.evaluate_path.size() == 1 &&
        annotation_entry.first.evaluate_path.back().is_property() &&
        annotation_entry.first.evaluate_path.back().to_property() ==
            "contains") {

      for (const auto &annotation_value : annotation_entry.second) {
        if (annotation_value.is_integer()) {
          passing_indices.insert(
              static_cast<std::size_t>(annotation_value.to_integer()));
        }
      }
    }
  }

  // Remove annotations for array items that didn't pass contains check
  if (!passing_indices.empty()) {
    for (auto it = this->annotations_.begin();
         it != this->annotations_.end();) {
      bool should_remove = false;

      // Check if this is a contains subschema annotation for a specific array
      // item
      if (!it->first.instance_location.empty() &&
          it->first.evaluate_path.size() > 1 &&
          it->first.evaluate_path.at(it->first.evaluate_path.size() - 2)
              .is_property() &&
          it->first.evaluate_path.at(it->first.evaluate_path.size() - 2)
                  .to_property() == "contains") {

        // Extract the array index from the instance location
        if (it->first.instance_location.size() == 1 &&
            it->first.instance_location.back().is_index()) {
          std::size_t array_index =
              it->first.instance_location.back().to_index();

          // Remove annotation if this array index didn't pass the contains
          // check
          if (passing_indices.find(array_index) == passing_indices.end()) {
            should_remove = true;
          }
        }
      }

      if (should_remove) {
        it = this->annotations_.erase(it);
      } else {
        ++it;
      }
    }
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
