#include <sourcemeta/blaze/output_simple.h>
#include <sourcemeta/blaze/output_standard.h>

#include <sourcemeta/core/jsonpointer.h>

#include <cassert>    // assert
#include <functional> // std::ref

namespace sourcemeta::blaze {

namespace {
auto weak_to_pointer(const sourcemeta::core::WeakPointer &weak)
    -> sourcemeta::core::Pointer {
  sourcemeta::core::Pointer result;
  for (const auto &token : weak) {
    if (token.is_property()) {
      result.push_back(token.to_property());
    } else {
      result.push_back(token.to_index());
    }
  }
  return result;
}
} // namespace

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
              const sourcemeta::core::PointerPositionTracker &positions,
              const StandardOutput format) -> sourcemeta::core::JSON {
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

        const auto position{
            positions.get(weak_to_pointer(annotation.first.instance_location))};
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

        const auto position{
            positions.get(weak_to_pointer(entry.instance_location))};
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
