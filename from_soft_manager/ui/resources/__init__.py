import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

def get_resource_path(*args: str) -> str:
    """
    Returns the absolute path to a resource file within the resources directory.

    Args:
        *args: Path components to append to the resources directory.

    Returns:
        str: The absolute path to the specified resource.

    """
    return os.path.join(CURRENT_DIR, *args)


def get_save_sound_path() -> str:
    """Absolute path to the save audio file.

    Returns:
        str: The absolute path to the save audio file.

    """
    return get_resource_path("audio", "soul_suck.wav")


def get_load_sound_path() -> str:
    """Absolute path to the load audio file.

    Returns:
        str: The absolute path to the load audio file.

    """
    return get_resource_path("audio", "ember_restored.wav")
