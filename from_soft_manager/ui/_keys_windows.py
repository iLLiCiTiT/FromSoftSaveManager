import platform

import win32api
from PySide6 import QtCore

VK_LBUTTON = 0x01  # Left mouse button
VK_RBUTTON = 0x02  # Right mouse button
VK_CANCEL = 0x03  # Control-break processing
VK_MBUTTON = 0x04  # Middle mouse button
VK_XBUTTON1 = 0x05  # X1 mouse button
VK_XBUTTON2 = 0x06  # X2 mouse button
VK_BACK = 0x08  # Backspace key
VK_TAB = 0x09  # Tab key
VK_CLEAR = 0x0C  # Clear key
VK_RETURN = 0x0D  # Enter key
VK_SHIFT = 0x10  # Shift key
VK_CONTROL = 0x11  # Ctrl key
VK_MENU = 0x12  # Alt key
VK_PAUSE = 0x13  # Pause key
VK_CAPITAL = 0x14  # Caps lock key
VK_ESCAPE = 0x1B  # Esc key
VK_SPACE = 0x20  # Spacebar key
VK_PRIOR = 0x21  # Page up key
VK_NEXT = 0x22  # Page down key
VK_END = 0x23  # End key
VK_HOME = 0x24  # Home key
VK_LEFT = 0x25  # Left arrow key
VK_UP = 0x26  # Up arrow key
VK_RIGHT = 0x27  # Right arrow key
VK_DOWN = 0x28  # Down arrow key
VK_SELECT = 0x29  # Select key
VK_PRINT = 0x2A  # Print key
VK_EXECUTE = 0x2B  # Execute key
VK_SNAPSHOT = 0x2C  # Print screen key
VK_INSERT = 0x2D  # Insert key
VK_DELETE = 0x2E  # Delete key
VK_HELP = 0x2F  # Help key
KEY_0 = 0x30  # 0 key
KEY_1 = 0x31  # 1 key
KEY_2 = 0x32  # 2 key
KEY_3 = 0x33  # 3 key
KEY_4 = 0x34  # 4 key
KEY_5 = 0x35  # 5 key
KEY_6 = 0x36  # 6 key
KEY_7 = 0x37  # 7 key
KEY_8 = 0x38  # 8 key
KEY_9 = 0x39  # 9 key
KEY_A = 0x41  # A key
KEY_B = 0x42  # B key
KEY_C = 0x43  # C key
KEY_D = 0x44  # D key
KEY_E = 0x45  # E key
KEY_F = 0x46  # F key
KEY_G = 0x47  # G key
KEY_H = 0x48  # H key
KEY_I = 0x49  # I key
KEY_J = 0x4A  # J key
KEY_K = 0x4B  # K key
KEY_L = 0x4C  # L key
KEY_M = 0x4D  # M key
KEY_N = 0x4E  # N key
KEY_O = 0x4F  # O key
KEY_P = 0x50  # P key
KEY_Q = 0x51  # Q key
KEY_R = 0x52  # R key
KEY_S = 0x53  # S key
KEY_T = 0x54  # T key
KEY_U = 0x55  # U key
KEY_V = 0x56  # V key
KEY_W = 0x57  # W key
KEY_X = 0x58  # X key
KEY_Y = 0x59  # Y key
KEY_Z = 0x5A  # Z key
VK_LWIN = 0x5B  # Left Windows logo key
VK_RWIN = 0x5C  # Right Windows logo key
VK_APPS = 0x5D  # Application key
VK_SLEEP = 0x5F  # Computer Sleep key
VK_NUMPAD0 = 0x60  # Numeric keypad 0 key
VK_NUMPAD1 = 0x61  # Numeric keypad 1 key
VK_NUMPAD2 = 0x62  # Numeric keypad 2 key
VK_NUMPAD3 = 0x63  # Numeric keypad 3 key
VK_NUMPAD4 = 0x64  # Numeric keypad 4 key
VK_NUMPAD5 = 0x65  # Numeric keypad 5 key
VK_NUMPAD6 = 0x66  # Numeric keypad 6 key
VK_NUMPAD7 = 0x67  # Numeric keypad 7 key
VK_NUMPAD8 = 0x68  # Numeric keypad 8 key
VK_NUMPAD9 = 0x69  # Numeric keypad 9 key
VK_MULTIPLY = 0x6A  # Multiply key
VK_ADD = 0x6B  # Add key
VK_SEPARATOR = 0x6C  # Separator key
VK_SUBTRACT = 0x6D  # Subtract key
VK_DECIMAL = 0x6E  # Decimal key
VK_DIVIDE = 0x6F  # Divide key
VK_F1 = 0x70  # F1 key
VK_F2 = 0x71  # F2 key
VK_F3 = 0x72  # F3 key
VK_F4 = 0x73  # F4 key
VK_F5 = 0x74  # F5 key
VK_F6 = 0x75  # F6 key
VK_F7 = 0x76  # F7 key
VK_F8 = 0x77  # F8 key
VK_F9 = 0x78  # F9 key
VK_F10 = 0x79  # F10 key
VK_F11 = 0x7A  # F11 key
VK_F12 = 0x7B  # F12 key
VK_F13 = 0x7C  # F13 key
VK_F14 = 0x7D  # F14 key
VK_F15 = 0x7E  # F15 key
VK_F16 = 0x7F  # F16 key
VK_F17 = 0x80  # F17 key
VK_F18 = 0x81  # F18 key
VK_F19 = 0x82  # F19 key
VK_F20 = 0x83  # F20 key
VK_F21 = 0x84  # F21 key
VK_F22 = 0x85  # F22 key
VK_F23 = 0x86  # F23 key
VK_F24 = 0x87  # F24 key
VK_NUMLOCK = 0x90  # Num lock key
VK_SCROLL = 0x91  # Scroll lock key
VK_LSHIFT = 0xA0  # Left Shift key
VK_RSHIFT = 0xA1  # Right Shift key
VK_LCONTROL = 0xA2  # Left Ctrl key
VK_RCONTROL = 0xA3  # Right Ctrl key
VK_LMENU = 0xA4  # Left Alt key
VK_RMENU = 0xA5  # Right Alt key
VK_BROWSER_BACK = 0xA6  # Browser Back key
VK_BROWSER_FORWARD = 0xA7  # Browser Forward key
VK_BROWSER_REFRESH = 0xA8  # Browser Refresh key
VK_BROWSER_STOP = 0xA9  # Browser Stop key
VK_BROWSER_SEARCH = 0xAA  # Browser Search key
VK_BROWSER_FAVORITES = 0xAB  # Browser Favorites key
VK_BROWSER_HOME = 0xAC  # Browser Start and Home key
VK_VOLUME_MUTE = 0xAD  # Volume Mute key
VK_VOLUME_DOWN = 0xAE  # Volume Down key
VK_VOLUME_UP = 0xAF  # Volume Up key
VK_MEDIA_NEXT_TRACK = 0xB0  # Next Track key
VK_MEDIA_PREV_TRACK = 0xB1  # Previous Track key
VK_MEDIA_STOP = 0xB2  # Stop Media key
VK_MEDIA_PLAY_PAUSE = 0xB3  # Play/Pause Media key
VK_LAUNCH_MAIL = 0xB4  # Start Mail key
VK_LAUNCH_MEDIA_SELECT = 0xB5  # Select Media key
VK_LAUNCH_APP1 = 0xB6  # Start Application 1 key
VK_LAUNCH_APP2 = 0xB7  # Start Application 2 key
VK_OEM_1 = 0xBA  # Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ;: key
VK_OEM_PLUS = 0xBB  # For any country/region, the + key
VK_OEM_COMMA = 0xBC  # For any country/region, the , key
VK_OEM_MINUS = 0xBD  # For any country/region, the - key
VK_OEM_PERIOD = 0xBE  # For any country/region, the . key
VK_OEM_2 = 0xBF  # Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the /? key
VK_OEM_3 = 0xC0  # Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the `~ key
VK_OEM_4 = 0xDB  # Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the [{ key
VK_OEM_5 = 0xDC  # Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the \\| key
VK_OEM_6 = 0xDD  # Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ]} key
VK_OEM_7 = 0xDE  # Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '" key
VK_OEM_8 = 0xDF  # Used for miscellaneous characters; it can vary by keyboard.
VK_OEM_102 = 0xE2  # The <> keys on the US standard keyboard, or the \\| key on the non-US 102-key keyboard

VK_PROCESSKEY = 0xE5  # IME PROCESS key

MAP_KEY_TO_QT = {
    VK_BACK: QtCore.Qt.Key_Backspace,
    VK_TAB: QtCore.Qt.Key_Tab,
    VK_CLEAR: QtCore.Qt.Key_Clear,
    VK_RETURN: QtCore.Qt.Key_Return,
    VK_SHIFT: QtCore.Qt.Key_Shift,
    VK_CONTROL: QtCore.Qt.Key_Control,
    VK_MENU: QtCore.Qt.Key_Alt,
    VK_PAUSE: QtCore.Qt.Key_Pause,
    VK_CAPITAL: QtCore.Qt.Key_CapsLock,
    VK_ESCAPE: QtCore.Qt.Key_Escape,
    VK_SPACE: QtCore.Qt.Key_Space,
    VK_PRIOR: QtCore.Qt.Key_PageUp,
    VK_NEXT: QtCore.Qt.Key_PageDown,
    VK_END: QtCore.Qt.Key_End,
    VK_HOME: QtCore.Qt.Key_Home,
    VK_LEFT: QtCore.Qt.Key_Left,
    VK_UP: QtCore.Qt.Key_Up,
    VK_RIGHT: QtCore.Qt.Key_Right,
    VK_DOWN: QtCore.Qt.Key_Down,
    VK_SELECT: QtCore.Qt.Key_Select,
    VK_PRINT: QtCore.Qt.Key_Print,
    VK_EXECUTE: QtCore.Qt.Key_Execute,
    # VK_SNAPSHOT: QtCore.Qt.,
    VK_INSERT: QtCore.Qt.Key_Insert,
    VK_DELETE: QtCore.Qt.Key_Delete,
    VK_HELP: QtCore.Qt.Key_Help,
    KEY_0: QtCore.Qt.Key_0,
    KEY_1: QtCore.Qt.Key_1,
    KEY_2: QtCore.Qt.Key_2,
    KEY_3: QtCore.Qt.Key_3,
    KEY_4: QtCore.Qt.Key_4,
    KEY_5: QtCore.Qt.Key_5,
    KEY_6: QtCore.Qt.Key_6,
    KEY_7: QtCore.Qt.Key_7,
    KEY_8: QtCore.Qt.Key_8,
    KEY_9: QtCore.Qt.Key_9,
    KEY_A: QtCore.Qt.Key_A,
    KEY_B: QtCore.Qt.Key_B,
    KEY_C: QtCore.Qt.Key_C,
    KEY_D: QtCore.Qt.Key_D,
    KEY_E: QtCore.Qt.Key_E,
    KEY_F: QtCore.Qt.Key_F,
    KEY_G: QtCore.Qt.Key_G,
    KEY_H: QtCore.Qt.Key_H,
    KEY_I: QtCore.Qt.Key_I,
    KEY_J: QtCore.Qt.Key_J,
    KEY_K: QtCore.Qt.Key_K,
    KEY_L: QtCore.Qt.Key_L,
    KEY_M: QtCore.Qt.Key_M,
    KEY_N: QtCore.Qt.Key_N,
    KEY_O: QtCore.Qt.Key_O,
    KEY_P: QtCore.Qt.Key_P,
    KEY_Q: QtCore.Qt.Key_Q,
    KEY_R: QtCore.Qt.Key_R,
    KEY_S: QtCore.Qt.Key_S,
    KEY_T: QtCore.Qt.Key_T,
    KEY_U: QtCore.Qt.Key_U,
    KEY_V: QtCore.Qt.Key_V,
    KEY_W: QtCore.Qt.Key_W,
    KEY_X: QtCore.Qt.Key_X,
    KEY_Y: QtCore.Qt.Key_Y,
    KEY_Z: QtCore.Qt.Key_Z,
    # VK_LWIN: QtCore.Qt.Key_Meta,
    # VK_RWIN: QtCore.Qt.Key_Meta,
    # VK_APPS: QtCore.Qt.,
    VK_SLEEP: QtCore.Qt.Key_Sleep,
    # VK_NUMPAD0: QtCore.Qt.,
    # VK_NUMPAD1: QtCore.Qt.,
    # VK_NUMPAD2: QtCore.Qt.,
    # VK_NUMPAD3: QtCore.Qt.,
    # VK_NUMPAD4: QtCore.Qt.,
    # VK_NUMPAD5: QtCore.Qt.,
    # VK_NUMPAD6: QtCore.Qt.,
    # VK_NUMPAD7: QtCore.Qt.,
    # VK_NUMPAD8: QtCore.Qt.,
    # VK_NUMPAD9: QtCore.Qt.,

    VK_MULTIPLY: QtCore.Qt.Key_Asterisk,
    VK_ADD: QtCore.Qt.Key_Plus,
    # VK_SEPARATOR: QtCore.Qt.,
    VK_SUBTRACT: QtCore.Qt.Key_Minus,
    VK_DECIMAL: QtCore.Qt.Key_Period,
    VK_DIVIDE: QtCore.Qt.Key_Slash,
    VK_F1: QtCore.Qt.Key_F1,
    VK_F2: QtCore.Qt.Key_F2,
    VK_F3: QtCore.Qt.Key_F3,
    VK_F4: QtCore.Qt.Key_F4,
    VK_F5: QtCore.Qt.Key_F5,
    VK_F6: QtCore.Qt.Key_F6,
    VK_F7: QtCore.Qt.Key_F7,
    VK_F8: QtCore.Qt.Key_F8,
    VK_F9: QtCore.Qt.Key_F9,
    VK_F10: QtCore.Qt.Key_F10,
    VK_F11: QtCore.Qt.Key_F11,
    VK_F12: QtCore.Qt.Key_F12,
    VK_F13: QtCore.Qt.Key_F13,
    VK_F14: QtCore.Qt.Key_F14,
    VK_F15: QtCore.Qt.Key_F15,
    VK_F16: QtCore.Qt.Key_F16,
    VK_F17: QtCore.Qt.Key_F17,
    VK_F18: QtCore.Qt.Key_F18,
    VK_F19: QtCore.Qt.Key_F19,
    VK_F20: QtCore.Qt.Key_F20,
    VK_F21: QtCore.Qt.Key_F21,
    VK_F22: QtCore.Qt.Key_F22,
    VK_F23: QtCore.Qt.Key_F23,
    VK_F24: QtCore.Qt.Key_F24,
    VK_NUMLOCK: QtCore.Qt.Key_NumLock,
    VK_SCROLL: QtCore.Qt.Key_ScrollLock,
    # VK_LSHIFT: QtCore.Qt.,
    # VK_RSHIFT: QtCore.Qt.,
    # VK_LCONTROL: QtCore.Qt.,
    # VK_RCONTROL: QtCore.Qt.,
    # VK_LMENU: QtCore.Qt.,
    # VK_RMENU: QtCore.Qt.,
    # VK_BROWSER_BACK: QtCore.Qt.,
    # VK_BROWSER_FORWARD: QtCore.Qt.,
    # VK_BROWSER_REFRESH: QtCore.Qt.,
    # VK_BROWSER_STOP: QtCore.Qt.,
    # VK_BROWSER_SEARCH: QtCore.Qt.,
    # VK_BROWSER_FAVORITES: QtCore.Qt.,
    # VK_BROWSER_HOME: QtCore.Qt.,
    # VK_VOLUME_MUTE: QtCore.Qt.,
    # VK_VOLUME_DOWN: QtCore.Qt.,
    # VK_VOLUME_UP: QtCore.Qt.,
    # VK_MEDIA_NEXT_TRACK: QtCore.Qt.,
    # VK_MEDIA_PREV_TRACK: QtCore.Qt.,
    # VK_MEDIA_STOP: QtCore.Qt.,
    # VK_MEDIA_PLAY_PAUSE: QtCore.Qt.,
    # VK_LAUNCH_MAIL: QtCore.Qt.,
    # VK_LAUNCH_MEDIA_SELECT: QtCore.Qt.,
    # VK_LAUNCH_APP1: QtCore.Qt.,
    # VK_LAUNCH_APP2: QtCore.Qt.,

    # VK_OEM_1: QtCore.Qt.,
    # VK_OEM_PLUS: QtCore.Qt.,
    # VK_OEM_COMMA: QtCore.Qt.,
    # VK_OEM_MINUS: QtCore.Qt.,
    # VK_OEM_PERIOD: QtCore.Qt.,
    # VK_OEM_2: QtCore.Qt.,
    # VK_OEM_3: QtCore.Qt.,
    # VK_OEM_4: QtCore.Qt.,
    # VK_OEM_5: QtCore.Qt.,
    # VK_OEM_6: QtCore.Qt.,
    # VK_OEM_7: QtCore.Qt.,
    # VK_OEM_8: QtCore.Qt.,
    # VK_OEM_102: QtCore.Qt.,
}
MAP_QT_TO_KEY = {
    value: key
    for key, value in MAP_KEY_TO_QT.items()
}


def qt_combination_to_int(keys: QtCore.QKeyCombination | None) -> set[int]:
    """Map Qt keys to Windows virtual key codes."""
    output = set()
    if keys is None:
        return output
    if keys.keyboardModifiers() & QtCore.Qt.ShiftModifier:
        output.add(VK_SHIFT)
    if keys.keyboardModifiers() & QtCore.Qt.ControlModifier:
        output.add(VK_CONTROL)
    if keys.keyboardModifiers() & QtCore.Qt.AltModifier:
        output.add(VK_MENU)

    key = keys.key()
    if not keys.keyboardModifiers() & QtCore.Qt.KeypadModifier:
        mapped_key = MAP_QT_TO_KEY.get(key)
        if mapped_key is None:
            return set()
        output.add(mapped_key)
        return output

    if key == QtCore.Qt.Key_0:
        output.add(VK_NUMPAD0)
    elif key == QtCore.Qt.Key_1:
        output.add(VK_NUMPAD1)
    elif key == QtCore.Qt.Key_2:
        output.add(VK_NUMPAD2)
    elif key == QtCore.Qt.Key_3:
        output.add(VK_NUMPAD3)
    elif key == QtCore.Qt.Key_4:
        output.add(VK_NUMPAD4)
    elif key == QtCore.Qt.Key_5:
        output.add(VK_NUMPAD5)
    elif key == QtCore.Qt.Key_6:
        output.add(VK_NUMPAD6)
    elif key == QtCore.Qt.Key_7:
        output.add(VK_NUMPAD7)
    elif key == QtCore.Qt.Key_8:
        output.add(VK_NUMPAD8)
    elif key == QtCore.Qt.Key_9:
        output.add(VK_NUMPAD9)
    return output


def int_combination_to_qt(keys: set[int]) -> QtCore.QKeyCombination | None:
    """Map Qt keys to Windows virtual key codes."""
    keys = set(keys)
    modifiers = QtCore.Qt.NoModifier
    if VK_SHIFT in keys:
        keys.discard(VK_SHIFT)
        modifiers |= QtCore.Qt.ShiftModifier
    if VK_CONTROL in keys:
        keys.discard(VK_CONTROL)
        modifiers |= QtCore.Qt.ControlModifier
    if VK_MENU in keys:
        keys.discard(VK_MENU)
        modifiers |= QtCore.Qt.AltModifier

    if len(keys) != 1:
        return None

    has_keypad = True
    key = None
    if VK_NUMPAD0 in keys:
        key = QtCore.Qt.Key_0
    elif VK_NUMPAD1 in keys:
        key = QtCore.Qt.Key_1
    elif VK_NUMPAD2 in keys:
        key = QtCore.Qt.Key_2
    elif VK_NUMPAD3 in keys:
        key = QtCore.Qt.Key_3
    elif VK_NUMPAD4 in keys:
        key = QtCore.Qt.Key_4
    elif VK_NUMPAD5 in keys:
        key = QtCore.Qt.Key_5
    elif VK_NUMPAD6 in keys:
        key = QtCore.Qt.Key_6
    elif VK_NUMPAD7 in keys:
        key = QtCore.Qt.Key_7
    elif VK_NUMPAD8 in keys:
        key = QtCore.Qt.Key_8
    elif VK_NUMPAD9 in keys:
        key = QtCore.Qt.Key_9
    else:
        has_keypad = False

    if has_keypad:
        modifiers |= QtCore.Qt.KeypadModifier
    else:
        key = MAP_KEY_TO_QT.get(next(iter(keys)))
    if key is None:
        return None
    return QtCore.QKeyCombination(modifiers, key)


def keys_are_pressed(keys: set[int]):
    if not keys:
        return False
    return all(
        win32api.GetAsyncKeyState(key)
        for key in keys
    )
