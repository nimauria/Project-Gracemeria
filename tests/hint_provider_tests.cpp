#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "gracemeria/hint_provider.hpp"

int main() {
  gracemeria::Ac6HintProvider provider;
  xenon::recomp::analysis::AnalysisHintSetV2 hint_set;
  std::string error;

  xenon::xbox::XexEffectiveIdentity supported{};
  supported.title_id = 0x4E4D07D1u;
  supported.media_id = 0x531C30BEu;
  assert(provider.provide(supported, hint_set, error));
  assert(error.empty());

  // These come from the real, on-disk revisions/usa-retail-tu0/analysis/
  // files - loaded, not hand-duplicated in this provider. hints.v2.json owns
  // the schema-native hooks/replacements/helpers and legacy-symbols.csv owns
  // the already-migrated AC6_recomp function-start table.
  assert(hint_set.native_replacements.size() == 16);
  assert(hint_set.runtime_helpers.size() == 2);
  assert(hint_set.indirect_calls.size() == 2);
  assert(hint_set.hooks.size() == 34);

  // Real-XEX validation showed Xenon's automatic discovery alone found only a
  // tiny subset of AC6. The revision profile therefore activates all 10,527
  // known AC6_recomp starts as FunctionHints.
  assert(hint_set.functions.size() == 10527);

  bool found_first_known_function = false;
  bool found_fallback_named_function = false;
  for (const auto& function : hint_set.functions) {
    if (function.address == 0x82090000u) {
      found_first_known_function = true;
      assert(function.name == "rex_sub_82090000");
    }
    // One of the 35 source entries that was written as `0x... = {}`.
    if (function.address == 0x820F5FB0u) {
      found_fallback_named_function = true;
      assert(!function.name.empty());
    }
  }
  assert(found_first_known_function);
  assert(found_fallback_named_function);

  // The provider must stamp the hint set with the EXACT identity it was
  // asked about (see hint_provider.hpp), so Xenon's own
  // hint_set_matches_identity() safety check passes.
  assert(xenon::recomp::analysis::hint_set_matches_identity(hint_set, supported));
  std::vector<std::string> schema_errors;
  assert(xenon::recomp::analysis::validate_hint_set(hint_set, schema_errors) && schema_errors.empty());

  hint_set = {};
  error.clear();
  auto wrong_media = supported;
  wrong_media.media_id = 0x36D871BCu;
  assert(!provider.provide(wrong_media, hint_set, error));
  assert(!error.empty());

  hint_set = {};
  error.clear();
  auto wrong_title = supported;
  wrong_title.title_id = 0x12345678u;
  assert(!provider.provide(wrong_title, hint_set, error));
  assert(!error.empty());

  hint_set = {};
  error.clear();
  auto effective_tu = supported;
  effective_tu.title_update_applied = true;
  assert(!provider.provide(effective_tu, hint_set, error));
  assert(!error.empty());

  // A revisions root with no matching revision directory is a hard failure,
  // never a silent empty/fallback hint set (no-stub rule).
  {
    const auto temp_root = std::filesystem::temp_directory_path() /
                            "gracemeria_hint_provider_tests_empty_root";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);
    gracemeria::Ac6HintProvider missing_data_provider(temp_root);
    xenon::recomp::analysis::AnalysisHintSetV2 empty_hint_set;
    std::string missing_error;
    assert(!missing_data_provider.provide(supported, empty_hint_set, missing_error));
    assert(!missing_error.empty());
    std::filesystem::remove_all(temp_root);
  }

  // A structurally malformed hints.v2.json is reported, never partially
  // applied. Use the actual revision directory name so this test exercises
  // malformed JSON rather than merely a missing directory.
  {
    const auto temp_root = std::filesystem::temp_directory_path() /
                            "gracemeria_hint_provider_tests_malformed_root";
    std::filesystem::remove_all(temp_root);
    const auto analysis_dir = temp_root / "usa-retail-tu0" / "analysis";
    std::filesystem::create_directories(analysis_dir);
    {
      std::ofstream out(analysis_dir / "hints.v2.json");
      out << "{ not valid json";
    }
    gracemeria::Ac6HintProvider malformed_provider(temp_root);
    xenon::recomp::analysis::AnalysisHintSetV2 malformed_hint_set;
    std::string malformed_error;
    assert(!malformed_provider.provide(supported, malformed_hint_set, malformed_error));
    assert(!malformed_error.empty());
    std::filesystem::remove_all(temp_root);
  }

  return 0;
}
