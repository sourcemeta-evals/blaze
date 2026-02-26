#include <sourcemeta/blaze/output_simple.h>
#include <sourcemeta/blaze/output_standard.h>

#include <cassert>    // assert
#include <functional> // std::ref

namespace sourcemeta::blaze {

namespace {

auto to_instance_position(
    const sourcemeta::core::PointerPositionTracker *const positions,
    const sourcemeta::core::WeakPointer &instance_location)
    -> std::optional<sourcemeta::core::PointerPositionTracker::Position> {
  if (positions == nullptr) {
    return std::nullopt;
  }

  return positions->get(sourcemeta::core::to_pointer(instance_location));
}

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const sourcemeta::core::PointerPositionTracker *const positions,
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
        if (const auto position{to_instance_position(
                positions, annotation.first.instance_location)};
            position.has_value()) {
          unit.assign("instancePosition",
                      sourcemeta::core::to_json(position.value()));
        }
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
        if (const auto position{
                to_instance_position(positions, entry.instance_location)};
            position.has_value()) {
          unit.assign("instancePosition",
                      sourcemeta::core::to_json(position.value()));
        }
        errors.push_back(std::move(unit));
      }

      assert(!errors.empty());
      result.assign("errors", std::move(errors));
      return result;
    }
  }
}

} // namespace

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format) -> sourcemeta::core::JSON {
  return standard(evaluator, schema, instance, nullptr, format);
}

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const sourcemeta::core::PointerPositionTracker &positions,
              const StandardOutput format) -> sourcemeta::core::JSON {
  return standard(evaluator, schema, instance, &positions, format);
}

} // namespace sourcemeta::blaze
