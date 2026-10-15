// Parses every entry of a Sekiro .sl2 as a record list and checks that
// re-serializing reproduces it byte for byte.
//
// Usage: SekiroRoundTrip <file.sl2> [more.sl2 ...]

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "../src/parse/SL2File.h"
#include "../src/parse/Sekiro/SaveData.h"

using namespace fssm::parse;

static bool check_file(const std::string& path) {
    std::printf("%s\n", path.c_str());
    SL2File sl2 = parse_sl2_file(path);
    if (sl2.game != fssm::Game::Sekiro) {
        std::printf("  not a Sekiro save, skipped\n");
        return true;
    }
    bool ok = true;
    for (const auto& entry : sl2.entries) {
        try {
            const auto parsed = sekiro::parse_entry(entry.content);
            const auto out = sekiro::serialize_entry(parsed);
            if (parsed.empty) {
                std::printf("  %s empty %s\n", entry.name.c_str(), out == entry.content ? "OK" : "MISMATCH");
            } else {
                std::string ids;
                for (const auto& r : parsed.records) ids += std::to_string(r.header.id) + " ";
                std::printf("  %s %s (records: %s, tail %zu bytes)\n", entry.name.c_str(),
                            out == entry.content ? "OK" : "MISMATCH", ids.c_str(), parsed.trailing.size());
            }
            if (out != entry.content) ok = false;
        } catch (const std::exception& e) {
            ok = false;
            std::printf("  %s ERROR %s\n", entry.name.c_str(), e.what());
        }
    }
    return ok;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: %s <file.sl2> [more.sl2 ...]\n", argv[0]);
        return 2;
    }
    bool ok = true;
    for (int i = 1; i < argc; ++i) {
        try {
            ok = check_file(argv[i]) && ok;
        } catch (const std::exception& e) {
            std::printf("  ERROR %s\n", e.what());
            ok = false;
        }
    }
    std::printf(ok ? "all entries round-trip exactly\n" : "FAILED\n");
    return ok ? 0 : 1;
}
