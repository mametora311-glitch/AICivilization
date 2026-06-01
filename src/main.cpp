#include "app/App.hpp"

int main() {
    aic::App app;
    if (!app.init()) {
        app.shutdown();
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
