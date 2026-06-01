#include "gui/GuiText.hpp"

#include <algorithm>
#include <vector>

namespace aic {

namespace {
Font gFont{};
bool gLoaded = false;

std::vector<int> makeCodepoints() {
    std::vector<int> cps;
    auto addCodepoint = [&](int c) {
        if (c > 0) cps.push_back(c);
    };
    auto addRange = [&](int first, int last) {
        for (int c = first; c <= last; ++c) addCodepoint(c);
    };
    auto addText = [&](const char* text) {
        const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
        while (*p) {
            int cp = 0;
            if (*p < 0x80) {
                cp = *p++;
            } else if ((*p & 0xe0) == 0xc0 && p[1]) {
                cp = ((*p & 0x1f) << 6) | (p[1] & 0x3f);
                p += 2;
            } else if ((*p & 0xf0) == 0xe0 && p[1] && p[2]) {
                cp = ((*p & 0x0f) << 12) | ((p[1] & 0x3f) << 6) | (p[2] & 0x3f);
                p += 3;
            } else if ((*p & 0xf8) == 0xf0 && p[1] && p[2] && p[3]) {
                cp = ((*p & 0x07) << 18) | ((p[1] & 0x3f) << 12) |
                     ((p[2] & 0x3f) << 6) | (p[3] & 0x3f);
                p += 4;
            } else {
                ++p;
            }
            addCodepoint(cp);
        }
    };
    addRange(0x0020, 0x007e); // ASCII
    addRange(0x2010, 0x203f); // punctuation variants
    addRange(0x2190, 0x21ff); // arrows
    addRange(0x3000, 0x303f); // Japanese punctuation
    addRange(0x3040, 0x309f); // Hiragana
    addRange(0x30a0, 0x30ff); // Katakana
    addRange(0xff00, 0xffef); // Fullwidth forms
    addText("→、。「」あいえかがきくげこさしじすずせただちってでとなにのはびへべまみよらりるれをんァィイェエオギクグコサシジスズセタダッツテデトドナネノパビフブプヘベペホボポマミムメモュョラリルレロワンー一上下不世中主了争亡交人他以仰件任会住体保信倉停傾先入全共内再出切列到制前創力動化協危原収口可台号合名向吸告命喰回団圧型域報場塔境壊声変外夢大失始存孤安完宗定実居展履崇崩工常干広序度庫庭延建強後復心応念急性情感態憶成戻房手承択抱持挙放教敬文断新族日明易時替最有未本果枯染検業概構標権止歩歴死残殖段殿水汚法注海消渇済渉減測源準滅灯点片物特状独理環生産由画界異発的知碑示社神秒秩移種穀立端管箱築系紛細終結継続維緊縮繁者耕能自至花荷落蝕行表裂複要規覧親観言記設話詳誕語説読調警護負責資起転農込送通速造進道達適選部重量録鎖鏡閉開間険階隔集雨面音順領頻飢餓験高（）");
    std::sort(cps.begin(), cps.end());
    cps.erase(std::unique(cps.begin(), cps.end()), cps.end());
    return cps;
}
} // namespace

void initGuiText() {
    if (gLoaded) return;
    static std::vector<int> codepoints = makeCodepoints();
    const char* candidates[] = {
        "C:/Windows/Fonts/NotoSansJP-VF.ttf",
        "assets/fonts/NotoSansJP-VF.ttf",
        "C:/Windows/Fonts/BIZ-UDGothicB.ttc",
        "C:/Windows/Fonts/meiryob.ttc",
        "C:/Windows/Fonts/meiryo.ttc",
        "C:/Windows/Fonts/BIZ-UDGothicR.ttc",
        "C:/Windows/Fonts/msgothic.ttc",
        "C:/Windows/Fonts/YuGothM.ttc",
        "C:/Windows/Fonts/NotoSansCJK-Regular.ttc"
    };
    for (const char* path : candidates) {
        if (!FileExists(path)) continue;
        gFont = LoadFontEx(path, 28, codepoints.data(), (int)codepoints.size());
        if (gFont.texture.id != 0) {
            SetTextureFilter(gFont.texture, TEXTURE_FILTER_BILINEAR);
            gLoaded = true;
            return;
        }
    }
    gFont = GetFontDefault();
    gLoaded = true;
}

Font guiFont() {
    initGuiText();
    return gFont;
}

void drawText(const char* text, int x, int y, int fontSize, Color color) {
    initGuiText();
    const float spacing = 0.0f;
    if (fontSize >= 22) {
        const float offset = fontSize >= 34 ? 2.0f : 1.0f;
        const unsigned char shadowAlpha =
            (unsigned char)std::min(150, std::max(60, (int)color.a - 75));
        DrawTextEx(gFont, text, { (float)x + offset, (float)y + offset },
                   (float)fontSize, spacing, Color{ 0, 0, 0, shadowAlpha });
    }
    if (fontSize >= 28) {
        DrawTextEx(gFont, text, { (float)x - 1.0f, (float)y + 1.0f },
                   (float)fontSize, spacing, Color{ 70, 90, 140, 70 });
    }
    DrawTextEx(gFont, text, { (float)x, (float)y }, (float)fontSize, spacing, color);
}

int measureText(const char* text, int fontSize) {
    initGuiText();
    return (int)MeasureTextEx(gFont, text, (float)fontSize, 0.0f).x;
}

} // namespace aic
