#include "notify/WindowsNotifier.hpp"

#include "util/Logger.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <iterator>
#include <string>

namespace aic {

#if defined(_WIN32)
namespace {

constexpr UINT kNotifyId = 1001;
constexpr UINT kNotifyMsg = WM_APP + 101;

LRESULT CALLBACK notifierWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return DefWindowProcW(hwnd, msg, wp, lp);
}

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) return L"";
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring out((size_t)len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), len);
    if (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

void copyWide(wchar_t* dst, size_t dstCount, const std::wstring& src) {
    if (dstCount == 0) return;
    wcsncpy_s(dst, dstCount, src.c_str(), _TRUNCATE);
}

} // namespace
#endif

bool WindowsNotifier::init() {
#if defined(_WIN32)
    HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = notifierWndProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"AICivilizationNotifierWindow";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"AICivilizationNotifier",
                                0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, inst, nullptr);
    if (!hwnd) {
        Logger::warn("WindowsNotifier: CreateWindowExW failed");
        return false;
    }

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = kNotifyId;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = kNotifyMsg;
    nid.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32516));
    copyWide(nid.szTip, std::size(nid.szTip), L"AICivilization");
    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
        Logger::warn("WindowsNotifier: Shell_NotifyIconW(NIM_ADD) failed");
        DestroyWindow(hwnd);
        return false;
    }
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
    hwnd_ = hwnd;
    added_ = true;
    Logger::info("WindowsNotifier initialized");
    return true;
#else
    Logger::warn("WindowsNotifier unavailable on this OS");
    return false;
#endif
}

void WindowsNotifier::shutdown() {
#if defined(_WIN32)
    if (added_ && hwnd_) {
        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = static_cast<HWND>(hwnd_);
        nid.uID = kNotifyId;
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }
    if (hwnd_) DestroyWindow(static_cast<HWND>(hwnd_));
#endif
    hwnd_ = nullptr;
    added_ = false;
}

bool WindowsNotifier::notify(const std::string& title, const std::string& message) {
#if defined(_WIN32)
    if (!added_ || !hwnd_) return false;
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = static_cast<HWND>(hwnd_);
    nid.uID = kNotifyId;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    copyWide(nid.szInfoTitle, std::size(nid.szInfoTitle), utf8ToWide(title));
    copyWide(nid.szInfo, std::size(nid.szInfo), utf8ToWide(message));
    const bool ok = Shell_NotifyIconW(NIM_MODIFY, &nid) != FALSE;
    if (!ok) Logger::warn("Windows notification failed: " + title);
    return ok;
#else
    (void)title;
    (void)message;
    return false;
#endif
}

} // namespace aic
