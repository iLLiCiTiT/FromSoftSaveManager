// Replaces files of the application with files of a new version and starts it again.
// Must not depend on anything in the application directory (Qt or MinGW dlls) because
//   those files are being replaced, so it uses only Windows api and is linked statically.
#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

static const wchar_t* TITLE = L"FromSoftSaveManager update";
static const wchar_t* BACKUP_DIRNAME = L".update_backup";
static const int FILE_ATTEMPTS = 40;
static const DWORD FILE_ATTEMPT_DELAY_MS = 500;
static const DWORD APP_EXIT_TIMEOUT_MS = 30000;

// Application files can be locked for a moment after exit (e.g. by antivirus)
template <typename Func>
static bool retry(Func func) {
    for (int attempt = 0; attempt < FILE_ATTEMPTS; ++attempt) {
        std::error_code ec;
        func(ec);
        if (!ec) return true;
        Sleep(FILE_ATTEMPT_DELAY_MS);
    }
    return false;
}

static void waitForExit(DWORD pid) {
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (process == nullptr) return;
    WaitForSingleObject(process, APP_EXIT_TIMEOUT_MS);
    CloseHandle(process);
}

static bool replaceFiles(const fs::path& source, const fs::path& target) {
    const fs::path backup = target / BACKUP_DIRNAME;
    std::error_code ec;
    fs::remove_all(backup, ec);

    std::vector<std::pair<fs::path, fs::path>> backedUp;
    std::vector<fs::path> created;
    bool success = true;

    for (fs::recursive_directory_iterator it(source, ec), end; success && !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file()) continue;
        const fs::path relative = fs::relative(it->path(), source);
        const fs::path dest = target / relative;
        fs::create_directories(dest.parent_path());

        if (fs::exists(dest)) {
            const fs::path saved = backup / relative;
            fs::create_directories(saved.parent_path());
            if (!retry([&](std::error_code& e) { fs::rename(dest, saved, e); })) {
                success = false;
                break;
            }
            backedUp.emplace_back(dest, saved);
        }
        created.push_back(dest);
        if (!retry([&](std::error_code& e) { fs::copy_file(it->path(), dest, e); })) {
            success = false;
        }
    }
    if (ec) success = false;

    if (success) {
        fs::remove_all(backup, ec);
        return true;
    }

    // Bring back the previous version
    for (const fs::path& path: created) fs::remove(path, ec);
    for (const auto& [dest, saved]: backedUp) fs::rename(saved, dest, ec);
    fs::remove_all(backup, ec);
    return false;
}

static void startApp(const fs::path& target, const std::wstring& exeName) {
    const fs::path exe = target / exeName;
    std::wstring commandLine = L"\"" + exe.wstring() + L"\"";
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, target.c_str(), &startupInfo, &processInfo)) {
        MessageBoxW(nullptr, L"The application could not be started. Please start it manually.", TITLE, MB_OK | MB_ICONERROR);
        return;
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // Arguments: <application pid> <source directory> <target directory> <executable name>
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr || argc != 5) {
        MessageBoxW(nullptr, L"The updater is started by the application and can't be used on its own.", TITLE, MB_OK | MB_ICONERROR);
        return 2;
    }
    const DWORD pid = static_cast<DWORD>(std::wcstoul(argv[1], nullptr, 10));
    const fs::path source = argv[2];
    const fs::path target = argv[3];
    const std::wstring exeName = argv[4];
    LocalFree(argv);

    waitForExit(pid);

    bool success = false;
    try {
        success = replaceFiles(source, target);
    } catch (const fs::filesystem_error&) {
        success = false;
    }
    if (!success) {
        MessageBoxW(
            nullptr,
            L"The update could not be installed and the previous version was kept.\n"
            L"Please make sure no other instance of the application is running and try again.",
            TITLE,
            MB_OK | MB_ICONERROR
        );
    }
    startApp(target, exeName);
    return success ? 0 : 1;
}
