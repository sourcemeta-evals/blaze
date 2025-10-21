#include <sourcemeta/blaze/output_simple.h>
#include <sourcemeta/blaze/output_standard.h>

#include <cassert>    // assert
#include <cstdint>    // std::uint64_t
#include <functional> // std::ref
#include <optional>   // std::optional
#include <tuple>      // std::tuple

namespace sourcemeta::blaze {

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format) -> sourcemeta::core::JSON {
  // We avoid a callback for this specific case for performance reasons
  if (format == StandardOutput::Flag) {
    auto result{sourcemeta::core::JSON::make_object()};
    const auto valid{evaluator.validate(schema, instance)};
    result.assign("valid", sourcemeta::core::JSON{valid});
    return result;
  } else {
    assert(format == StandardOutput::Basic);
    SimpleOutput output{instance};
    const auto valid{evaluator.validate(schema, instance, std::ref(output))};

    if (valid) {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto annotations{sourcemeta::core::JSON::make_array()};
      for (const auto &annotation : output.annotations()) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(annotation.first.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{annotation.first.schema_location});
        unit.assign(
            "instanceLocation",
            sourcemeta::core::to_json(annotation.first.instance_location));
        unit.assign("annotation", sourcemeta::core::to_json(annotation.second));
        annotations.push_back(std::move(unit));
      }

      if (!annotations.empty()) {
        result.assign("annotations", std::move(annotations));
      }

      return result;
    } else {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto errors{sourcemeta::core::JSON::make_array()};
      for (const auto &entry : output) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(entry.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{entry.schema_location});
        unit.assign("instanceLocation",
                    sourcemeta::core::to_json(entry.instance_location));
        unit.assign("error", sourcemeta::core::JSON{entry.message});
        errors.push_back(std::move(unit));
      }

      assert(!errors.empty());
      result.assign("errors", std::move(errors));
      return result;
    }
  }
}

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const sourcemeta::core::PointerPositionTracker &tracker,
              const StandardOutput format) -> sourcemeta::core::JSON {
  // We avoid a callback for this specific case for performance reasons
  if (format == StandardOutput::Flag) {
    auto result{sourcemeta::core::JSON::make_object()};
    const auto valid{evaluator.validate(schema, instance)};
    result.assign("valid", sourcemeta::core::JSON{valid});
    return result;
  } else {
    assert(format == StandardOutput::Basic);
    SimpleOutput output{instance};
    const auto valid{evaluator.validate(schema, instance, std::ref(output))};

    if (valid) {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto annotations{sourcemeta::core::JSON::make_array()};
      for (const auto &annotation : output.annotations()) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(annotation.first.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{annotation.first.schema_location});
        unit.assign(
            "instanceLocation",
            sourcemeta::core::to_json(annotation.first.instance_location));

        // Add instancePosition if available
        // Convert WeakPointer to Pointer for tracker lookup
        sourcemeta::core::Pointer instance_pointer;
        for (const auto &token : annotation.first.instance_location) {
          if (token.is_property()) {
            instance_pointer.emplace_back(token.to_property());
          } else {
            instance_pointer.emplace_back(token.to_index());
          }
        }
        const auto position{tracker.get(instance_pointer)};
        if (position.has_value()) {
          unit.assign("instancePosition",
                      sourcemeta::core::to_json(position.value()));
        }

        unit.assign("annotation", sourcemeta::core::to_json(annotation.second));
        annotations.push_back(std::move(unit));
      }

      if (!annotations.empty()) {
        result.assign("annotations", std::move(annotations));
      }

      return result;
    } else {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto errors{sourcemeta::core::JSON::make_array()};
      for (const auto &entry : output) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(entry.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{entry.schema_location});
        unit.assign("instanceLocation",
                    sourcemeta::core::to_json(entry.instance_location));

        // Add instancePosition if available
        // Convert WeakPointer to Pointer for tracker lookup
        sourcemeta::core::Pointer instance_pointer;
        for (const auto &token : entry.instance_location) {
          if (token.is_property()) {
            instance_pointer.emplace_back(token.to_property());
          } else {
            instance_pointer.emplace_back(token.to_index());
          }
        }
        const auto position{tracker.get(instance_pointer)};
        if (position.has_value()) {
          unit.assign("instancePosition",
                      sourcemeta::core::to_json(position.value()));
        }

        unit.assign("error", sourcemeta::core::JSON{entry.message});
        errors.push_back(std::move(unit));
      }

      assert(!errors.empty());
      result.assign("errors", std::move(errors));
      return result;
    }
  }
}

} // namespace sourcemeta::blaze
