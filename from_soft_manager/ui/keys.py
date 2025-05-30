import platform


def keys_are_pressed(keys: set[int]):
    return False


if platform.system().lower() == "windows":
    import win32api


    def keys_are_pressed(keys: set[int]):
        if not keys:
            return False
        return all(
            win32api.GetAsyncKeyState(key)
            for key in keys
        )
