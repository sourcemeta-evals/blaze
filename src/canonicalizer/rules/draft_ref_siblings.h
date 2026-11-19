class DraftRefSiblings final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DraftRefSiblings() : SchemaTransformRule{"draft_ref_siblings"} {}

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$ref"));

    // What a reference overrides at the top of a document includes the
    // dialect the document names and the container the reference itself goes
    // through, so reading such a document means picking which of the two to
    // honour. Taking the name on trust is a guess about a document that says
    // nothing anyone may read, and dropping it leaves the reference pointing
    // at a container that is no longer there. Bundling in the dependency
    // refuses the same spelling rather than choose
    if (location.pointer.empty() && schema.size() > 1) {
      throw SchemaError("Cannot canonicalise a JSON Schema Draft 7 or older "
                        "with a top-level `$ref` (which overrides sibling "
                        "keywords) without introducing undefined behavior");
    }

    std::vector<sourcemeta::core::Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::core::SchemaKeywordType::Reference ||
          metadata.type == sourcemeta::core::SchemaKeywordType::Comment ||
          // If we disallow this, we end up deleting it and the linter will fail
          // with an error about not knowing the dialect
          entry.first == "$schema") {
        continue;
      }
      locations.push_back(sourcemeta::core::Pointer{entry.first});
    }

    ONLY_CONTINUE_IF(!locations.empty());
    this->locations_ = std::move(locations);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    for (const auto &location : this->locations_) {
      schema.erase(location.at(0).to_property());
    }
  }

private:
  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
