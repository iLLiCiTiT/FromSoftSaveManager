#pragma once

// Minimal Windows VK <-> Qt key mapping and hotkey utilities.
// Ported from python/from_soft_manager/ui/_keys_windows.py

#include <unordered_set>

#include <QKeyCombination>
#include <Qt>

// Convert a Qt key combination to a set of Windows virtual-key codes (ints).
// Returns empty set if input is invalid or not supported.
std::unordered_set<int> qtCombinationToInt(const QKeyCombination& keys);

// Convert a set of Windows virtual-key codes to a Qt key combination.
// Returns std::nullopt if unsupported or ambiguous (requires exactly 1 non-modifier key).
QKeyCombination intCombinationToQt(const std::unordered_set<int>& vkCodes);

// Check whether all specified virtual-key codes are pressed at the moment.
// Uses GetAsyncKeyState under the hood (on Windows only).
bool keysArePressed(const std::unordered_set<int>& vkCodes);
