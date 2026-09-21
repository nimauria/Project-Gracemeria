#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "xenon/core/session.hpp"
#include "xenon/xbox/xex_loader.hpp"

namespace {
const char* status_name(const xenon::core::ExportDescriptor* desc) {
  if (!desc) return "missing";
  using R = xenon::core::ExportRequirement;
  if (desc->requirement == R::Stubbed) return "stub";
  return desc->handler ? "registered" : "missing";
}
}

int main(int argc, char** argv) {
  if (argc != 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
    std::cout << "usage: gracemeria-import-coverage <default.xex>\n";
    return argc == 2 ? 0 : 1;
  }
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::cerr << "unable to open XEX\n";
    return 2;
  }
  const std::vector<char> raw((std::istreambuf_iterator<char>(file)), {});
  std::vector<std::byte> bytes(raw.size());
  std::transform(raw.begin(), raw.end(), bytes.begin(), [](char c) {
    return static_cast<std::byte>(static_cast<unsigned char>(c));
  });

  xenon::xbox::XexImage image;
  std::string error;
  if (!xenon::xbox::parse_xex_image(bytes, image, &error)) {
    std::cerr << error << '\n';
    return 3;
  }

  xenon::core::XenonSession session;
  xenon::core::SessionConfig config;
  config.enable_graphics = false;
  config.enable_input = false;
  config.enable_audio = false;
  config.enable_network = false;
  const auto initialized = session.initialize(config);
  if (!initialized.success) {
    std::cerr << initialized.message << '\n';
    return 4;
  }

  std::size_t registered = 0, stubbed = 0, missing = 0;
  std::cout << "library\tordinal\tname\tstatus\n";
  for (const auto& item : image.imports) {
    const auto* desc = item.symbol.empty()
                           ? session.exports()->resolve(item.module, item.ordinal)
                           : session.exports()->resolve(item.module, item.symbol);
    if (!desc && !item.symbol.empty()) desc = session.exports()->resolve(item.module, item.ordinal);
    const auto* status = status_name(desc);
    if (std::string_view(status) == "registered") ++registered;
    else if (std::string_view(status) == "stub") ++stubbed;
    else ++missing;
    std::cout << item.module << '\t' << item.ordinal << '\t'
              << (item.symbol.empty() ? (desc ? desc->name : std::string{}) : item.symbol)
              << '\t' << status << '\n';
  }
  std::cout << "summary\tregistered=" << registered << "\tstub=" << stubbed
            << "\tmissing=" << missing << '\n';
  session.shutdown();
  return missing == 0 ? 0 : 5;
}
