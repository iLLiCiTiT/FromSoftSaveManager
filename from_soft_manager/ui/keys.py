import platform


def get_pressed_keys():
    return set()


if platform.system().lower() == "windows":
    import win32api

    def get_pressed_keys():
        return {
            idx
            for idx in range(3, 256)
            if win32api.GetAsyncKeyState(idx)
        }
