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
    this->mask.erase(evaluate_path);
  }

  if (result) {
    return;
  }

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    const std::string &kw_loc = step.keyword_location;

    // Only prune annotations for contains failures if this is a direct child of
    // contains and we're dealing with a type/validation failure, not the
    // contains instruction itself
    const auto contains_pos = kw_loc.rfind("/contains/");
    if (contains_pos != std::string::npos) {
      // This is a failure within a contains subschema (e.g., /contains/type)
      // Find the contains scope (everything up to and including "/contains")
      const std::string contains_scope =
          kw_loc.substr(0, contains_pos + std::string("/contains").size());

      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        const bool same_item =
            (iterator->first.instance_location == instance_location);
        const bool same_contains_scope =
            iterator->first.schema_location.rfind(contains_scope, 0) == 0;

        if (same_item && same_contains_scope) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    } else {
      for (auto iterator = this->annotations_.begin();
           iterator != this->annotations_.end();) {
        if (iterator->first.evaluate_path.starts_with_initial(
                effective_evaluate_path)) {
          iterator = this->annotations_.erase(iterator);
        } else {
          iterator++;
        }
      }
    }
  }

  if (std::any_of(this->mask.cbegin(), this->mask.cend(),
                  [&evaluate_path](const auto &entry) {
                    return evaluate_path.starts_with(entry.first) &&
                           !entry.second;
                  })) {
    return;
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
