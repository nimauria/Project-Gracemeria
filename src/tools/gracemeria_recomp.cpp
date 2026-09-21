#include <filesystem>
#include <iostream>
#include <string>

#include "gracemeria/hint_provider.hpp"
#include "xenon/recomp/driver.hpp"

namespace {
void usage() {
  std::cout << "usage: gracemeria-recomp <default.xex> [output-directory] [--json]\n"
               "  Title updates are Xenon runtime concerns (XenonSession::load_game() ->\n"
               "  xbox::apply_title_update()), not a Recomp Driver/static-analysis input -\n"
               "  the driver only ever analyzes the base executable given here. Gracemeria\n"
               "  deliberately does not accept a title update on this CLI (see\n"
               "  Ac6HintProvider - a title-update effective image needs its own verified\n"
               "  revision profile before Gracemeria supplies hints for it).\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
    usage();
    return argc < 2 ? 1 : 0;
  }

  xenon::recomp::DriverOptions options;
  options.input = argv[1];
  options.output = "generated";
  bool json = false;
  bool output_set = false;
  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--json") {
      json = true;
    } else if (!output_set && !arg.starts_with("--")) {
      options.output = arg;
      output_set = true;
    } else {
      std::cerr << "gracemeria-recomp: unknown argument: " << arg << '\n';
      usage();
      return 1;
    }
  }

  gracemeria::Ac6HintProvider provider;
  options.hint_provider_v2 = &provider;

  xenon::recomp::AnalysisReport report;
  std::string error;
  if (!xenon::recomp::load_and_analyze(options, report, error)) {
    std::cerr << "gracemeria-recomp: " << error << '\n';
    return 2;
  }
  if (!xenon::recomp::generate_project(options, report, error)) {
    std::cerr << "gracemeria-recomp: " << error << '\n';
    return 3;
  }
  std::cout << (json ? xenon::recomp::format_report_json(report)
                     : xenon::recomp::format_report(report));
  return report.unresolved.empty() ? 0 : 4;
}
