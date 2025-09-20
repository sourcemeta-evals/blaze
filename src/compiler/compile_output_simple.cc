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

    // Special handling for contains completion
    if (!evaluate_path.empty() && evaluate_path.back().is_property() &&
        evaluate_path.back().to_property() == "contains") {

      // When contains evaluation completes, we need to clean up annotations
      // for instances that didn't contribute to the contains success
      if (result) {
        // Contains succeeded - we need to determine which instances contributed
        // For now, let's implement a simple heuristic: if the contains
        // annotation exists, use its value to determine successful instances

        // Find the contains annotation to see which instances matched
        sourcemeta::core::WeakPointer contains_instance_path;
        if (!instance_location.empty()) {
          // We're evaluating contains on a specific instance
          contains_instance_path = instance_location;
        }

        Location contains_annotation_location{
            contains_instance_path, evaluate_path, step.keyword_location};
        auto contains_match =
            this->annotations_.find(contains_annotation_location);

        if (contains_match != this->annotations_.end() &&
            !contains_match->second.empty()) {
          // The contains annotation should contain the index of the matching
          // item
          auto contains_value = contains_match->second.back();
          if (contains_value.is_integer()) {
            std::size_t successful_index =
                static_cast<std::size_t>(contains_value.to_integer());
            std::cerr << "  Contains successful index: " << successful_index
                      << std::endl;

            // Remove annotations for all other instances within this contains
            // subtree
            for (auto iterator = this->annotations_.begin();
                 iterator != this->annotations_.end();) {
              const auto &location = iterator->first;

              // Check if this annotation is within the contains subtree
              if (location.evaluate_path.starts_with_initial(evaluate_path) &&
                  location.evaluate_path != evaluate_path) {

                // Extract the instance index from the instance location
                if (!location.instance_location.empty() &&
                    location.instance_location.back().is_index()) {
                  std::size_t annotation_index =
                      location.instance_location.back().to_index();

                  std::cerr << "    Checking annotation at index "
                            << annotation_index
                            << " (successful: " << successful_index << ")"
                            << std::endl;

                  if (annotation_index != successful_index) {
                    std::cerr
                        << "      REMOVING annotation for failed contains item"
                        << std::endl;
                    iterator = this->annotations_.erase(iterator);
                    continue;
                  }
                }
              }
              iterator++;
            }
          }
        }
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

  if (type == EvaluationType::Post && !result && !this->annotations_.empty()) {
    std::cerr << "CLEANUP: Post failure at instance "
              << sourcemeta::core::to_string(instance_location) << " evaluate "
              << sourcemeta::core::to_string(evaluate_path)
              << " result=" << result << std::endl;

    // Check if this failure is within a contains subschema
    bool is_within_contains = false;
    auto contains_path = evaluate_path;

    // Walk up the evaluate path to find if we're within a contains evaluation
    while (!contains_path.empty()) {
      if (contains_path.back().is_property() &&
          contains_path.back().to_property() == "contains") {
        is_within_contains = true;
        break;
      }
      contains_path.pop_back();
    }

    if (is_within_contains) {
      std::cerr << "  Found contains subschema at path: "
                << sourcemeta::core::to_string(contains_path) << std::endl;
      // For contains subschema failures, remove annotations for this specific
      // instance that are within the contains subtree
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        const auto &location = iterator->first;

        std::cerr << "    Checking annotation: instance "
                  << sourcemeta::core::to_string(location.instance_location)
                  << " evaluate "
                  << sourcemeta::core::to_string(location.evaluate_path)
                  << std::endl;

        // Remove annotations for the same instance that are within the contains
        // subtree
        if (location.instance_location == instance_location &&
            location.evaluate_path.starts_with_initial(contains_path)) {
          std::cerr << "      REMOVING annotation (contains cleanup)"
                    << std::endl;
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    } else {
      // Original cleanup logic for non-contains cases
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        if (iterator->first.evaluate_path.starts_with_initial(evaluate_path)) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
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
