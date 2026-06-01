#pragma once

#include "raylib.h"

namespace aic {

void initGuiText();
Font guiFont();
void drawText(const char* text, int x, int y, int fontSize, Color color);
int measureText(const char* text, int fontSize);

} // namespace aic
