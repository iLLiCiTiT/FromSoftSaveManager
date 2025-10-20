#include "KeysWindows.h"

#define NOMINMAX
#include <windows.h>
#undef NOMINMAX

#include <unordered_map>

namespace {

// Map Windows VK to Qt key (for non-keypad keys mostly)
const std::unordered_map<int, Qt::Key> MAP_KEY_TO_QT = {
    {VK_BACK, Qt::Key_Backspace},
    {VK_TAB, Qt::Key_Tab},
    {VK_CLEAR, Qt::Key_Clear},
    {VK_RETURN, Qt::Key_Return},
    {VK_SHIFT, Qt::Key_Shift},
    {VK_CONTROL, Qt::Key_Control},
    {VK_MENU, Qt::Key_Alt},
    {VK_PAUSE, Qt::Key_Pause},
    {VK_CAPITAL, Qt::Key_CapsLock},
    {VK_ESCAPE, Qt::Key_Escape},
    {VK_SPACE, Qt::Key_Space},
    {VK_PRIOR, Qt::Key_PageUp},
    {VK_NEXT, Qt::Key_PageDown},
    {VK_END, Qt::Key_End},
    {VK_END, Qt::Key_End},
    {VK_HOME, Qt::Key_Home},
    {VK_LEFT, Qt::Key_Left},
    {VK_UP, Qt::Key_Up},
    {VK_RIGHT, Qt::Key_Right},
    {VK_DOWN, Qt::Key_Down},
    {VK_SELECT, Qt::Key_Select},
    {VK_PRINT, Qt::Key_Print},
    {VK_EXECUTE, Qt::Key_Execute},
    {VK_INSERT, Qt::Key_Insert},
    {VK_DELETE, Qt::Key_Delete},
    {VK_HELP, Qt::Key_Help},

    // 0-9
    {0x30, Qt::Key_0},
    {0x31, Qt::Key_1},
    {0x32, Qt::Key_2},
    {0x33, Qt::Key_3},
    {0x34, Qt::Key_4},
    {0x35, Qt::Key_5},
    {0x36, Qt::Key_6},
    {0x37, Qt::Key_7},
    {0x38, Qt::Key_8},
    {0x39, Qt::Key_9},

    // A-Z
    {0x41, Qt::Key_A}, {0x42, Qt::Key_B}, {0x43, Qt::Key_C}, {0x44, Qt::Key_D},
    {0x45, Qt::Key_E}, {0x46, Qt::Key_F}, {0x47, Qt::Key_G}, {0x48, Qt::Key_H},
    {0x49, Qt::Key_I}, {0x4A, Qt::Key_J}, {0x4B, Qt::Key_K}, {0x4C, Qt::Key_L},
    {0x4D, Qt::Key_M}, {0x4E, Qt::Key_N}, {0x4F, Qt::Key_O}, {0x50, Qt::Key_P},
    {0x51, Qt::Key_Q}, {0x52, Qt::Key_R}, {0x53, Qt::Key_S}, {0x54, Qt::Key_T},
    {0x55, Qt::Key_U}, {0x56, Qt::Key_V}, {0x57, Qt::Key_W}, {0x58, Qt::Key_X},
    {0x59, Qt::Key_Y}, {0x5A, Qt::Key_Z},

    {VK_SLEEP, Qt::Key_Sleep},

    // Numpad arithmetic and F-keys
    {VK_MULTIPLY, Qt::Key_Asterisk},
    {VK_ADD, Qt::Key_Plus},
    {VK_SUBTRACT, Qt::Key_Minus},
    {VK_DECIMAL, Qt::Key_Period},
    {VK_DIVIDE, Qt::Key_Slash},

    {VK_F1, Qt::Key_F1}, {VK_F2, Qt::Key_F2}, {VK_F3, Qt::Key_F3}, {VK_F4, Qt::Key_F4},
    {VK_F5, Qt::Key_F5}, {VK_F6, Qt::Key_F6}, {VK_F7, Qt::Key_F7}, {VK_F8, Qt::Key_F8},
    {VK_F9, Qt::Key_F9}, {VK_F10, Qt::Key_F10}, {VK_F11, Qt::Key_F11}, {VK_F12, Qt::Key_F12},
    {VK_F13, Qt::Key_F13}, {VK_F14, Qt::Key_F14}, {VK_F15, Qt::Key_F15}, {VK_F16, Qt::Key_F16},
    {VK_F17, Qt::Key_F17}, {VK_F18, Qt::Key_F18}, {VK_F19, Qt::Key_F19}, {VK_F20, Qt::Key_F20},
    {VK_F21, Qt::Key_F21}, {VK_F22, Qt::Key_F22}, {VK_F23, Qt::Key_F23}, {VK_F24, Qt::Key_F24},

    {VK_NUMLOCK, Qt::Key_NumLock},
    {VK_SCROLL, Qt::Key_ScrollLock},
};

// Reverse map: Qt key -> Windows VK (only for non-keypad alphanumerics and function keys)
const std::unordered_map<int, int> MAP_QT_TO_KEY = [] {
    std::unordered_map<int, int> m;
    for (const auto& kv : MAP_KEY_TO_QT) {
        m.emplace(static_cast<int>(kv.second), kv.first);
    }
    return m;
}();

}

std::set<int> qtCombinationToInt(const QKeyCombination& combo) {
    std::set<int> out;

    Qt::KeyboardModifiers mods = combo.keyboardModifiers();
    if (mods & Qt::ShiftModifier) out.insert(VK_SHIFT);
    if (mods & Qt::ControlModifier) out.insert(VK_CONTROL);
    if (mods & Qt::AltModifier) out.insert(VK_MENU);

    // Main key
    int key = combo.key();

    // Handle keypad special-case: if KeypadModifier is present and key is 0..9, map to VK_NUMPADx
    if (mods & Qt::KeypadModifier) {
        switch (key) {
            case Qt::Key_0: out.insert(VK_NUMPAD0); return out;
            case Qt::Key_1: out.insert(VK_NUMPAD1); return out;
            case Qt::Key_2: out.insert(VK_NUMPAD2); return out;
            case Qt::Key_3: out.insert(VK_NUMPAD3); return out;
            case Qt::Key_4: out.insert(VK_NUMPAD4); return out;
            case Qt::Key_5: out.insert(VK_NUMPAD5); return out;
            case Qt::Key_6: out.insert(VK_NUMPAD6); return out;
            case Qt::Key_7: out.insert(VK_NUMPAD7); return out;
            case Qt::Key_8: out.insert(VK_NUMPAD8); return out;
            case Qt::Key_9: out.insert(VK_NUMPAD9); return out;
            default: break; // fallthrough to generic mapping
        }
    }

    auto it = MAP_QT_TO_KEY.find(key);
    if (it == MAP_QT_TO_KEY.end()) {
        // Unsupported key
        return {};
    }
    out.insert(it->second);
    return out;
}

std::optional<QKeyCombination> intCombinationToQt(const std::set<int>& in) {
    // Copy and strip modifiers
    std::set<int> keys = in;
    Qt::KeyboardModifiers mods = Qt::NoModifier;

    if (keys.erase(VK_SHIFT)) mods |= Qt::ShiftModifier;
    if (keys.erase(VK_CONTROL)) mods |= Qt::ControlModifier;
    if (keys.erase(VK_MENU)) mods |= Qt::AltModifier;

    if (keys.size() != 1) return std::nullopt; // ambiguous or missing

    bool hasKeypad = true;
    int qtKey = Qt::Key_unknown;

    int vk = *keys.begin();
    switch (vk) {
        case VK_NUMPAD0: qtKey = Qt::Key_0; break;
        case VK_NUMPAD1: qtKey = Qt::Key_1; break;
        case VK_NUMPAD2: qtKey = Qt::Key_2; break;
        case VK_NUMPAD3: qtKey = Qt::Key_3; break;
        case VK_NUMPAD4: qtKey = Qt::Key_4; break;
        case VK_NUMPAD5: qtKey = Qt::Key_5; break;
        case VK_NUMPAD6: qtKey = Qt::Key_6; break;
        case VK_NUMPAD7: qtKey = Qt::Key_7; break;
        case VK_NUMPAD8: qtKey = Qt::Key_8; break;
        case VK_NUMPAD9: qtKey = Qt::Key_9; break;
        default:
            hasKeypad = false;
            break;
    }

    if (hasKeypad) {
        mods |= Qt::KeypadModifier;
    } else {
        auto it = MAP_KEY_TO_QT.find(vk);
        if (it == MAP_KEY_TO_QT.end()) return std::nullopt;
        qtKey = static_cast<int>(it->second);
    }

    if (qtKey == Qt::Key_unknown) return std::nullopt;
    return QKeyCombination(mods, static_cast<Qt::Key>(qtKey));
}

bool keysArePressed(const std::set<int>& vkCodes) {
    if (vkCodes.empty()) return false;
    for (int vk : vkCodes) {
        SHORT state = GetAsyncKeyState(vk);
        if ((state & 0x8000) == 0) return false; // high bit set when key is down
    }
    return true;
}
