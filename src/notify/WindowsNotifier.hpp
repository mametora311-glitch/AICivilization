#pragma once

#include <string>

namespace aic {

class WindowsNotifier {
public:
    bool init();
    void shutdown();
    bool notify(const std::string& title, const std::string& message);

private:
    void* hwnd_ = nullptr;
    bool added_ = false;
};

} // namespace aic
