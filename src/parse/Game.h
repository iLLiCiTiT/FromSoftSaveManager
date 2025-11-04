#pragma once


namespace fssm {
    class Game {
    public:
        enum Value: uint8_t {
            Unknown,
            DSR,
            DS2_SOTFS,
            DS3,
            ER,
            Sekiro,
        };

        Game() = default;
        constexpr Game(Value aGame) : value(aGame) {}

        // Allow switch and comparisons.
        constexpr operator Value() const {return value;}

        // Prevent usage: if(game)
        explicit operator bool() const = delete;

        constexpr bool operator==(Game a) const {return value == a.value;}
        constexpr bool operator==(Value a) const {return value == a;}
        constexpr bool operator!=(Game a) const {return value != a.value;}
        constexpr bool operator!=(Value a) const {return value != a;}

        constexpr const char* toString() const {
            switch (value) {
                case DSR: return "DSR";
                case DS2_SOTFS: return "DS2-SOTFS";
                case DS3: return "DS3";
                case ER: return "ER";
                case Sekiro: return "Sekiro";
                default: return "Unknown";
            }
        }

        static Game fromString(std::string_view gameName) {
            if (iequals(gameName, "dsr"))       return DSR;
            if (iequals(gameName, "ds3"))       return DS3;
            if (iequals(gameName, "er"))        return ER;
            if (iequals(gameName, "sekiro"))    return Sekiro;
            if (iequals(gameName, "ds2-sotfs")) return DS2_SOTFS;
            return Unknown;
        }

    private:
        Value value;
        static bool iequals(std::string_view a, std::string_view b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                unsigned char ca = static_cast<unsigned char>(a[i]);
                unsigned char cb = static_cast<unsigned char>(b[i]);
                if (std::tolower(ca) != cb) return false;
            }
            return true;
        }
    };
}
