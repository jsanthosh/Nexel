#include "NVTheme.h"

namespace nv {

NVTheme NVTheme::excel() {
    NVTheme t;
    t.name = "Excel";
    t.seriesColors = {
        colorHex(0x4472C4), colorHex(0xED7D31), colorHex(0xA5A5A5),
        colorHex(0xFFC000), colorHex(0x5B9BD5), colorHex(0x70AD47),
        colorHex(0x264478), colorHex(0x9B57A0), colorHex(0x636363)
    };
    t.plotAreaColor = {1, 1, 1, 1};
    t.gridLineColor = {0.85f, 0.85f, 0.85f, 1};
    t.lineWidth = 2.5f;
    t.barCornerRadius = 0;
    return t;
}

NVTheme NVTheme::material() {
    NVTheme t;
    t.name = "Material";
    t.seriesColors = {
        colorHex(0x1976D2), colorHex(0xD32F2F), colorHex(0x388E3C),
        colorHex(0xF57C00), colorHex(0x7B1FA2), colorHex(0x00796B),
        colorHex(0x5D4037), colorHex(0x455A64), colorHex(0xC2185B)
    };
    t.plotAreaColor = colorHex(0xFAFAFA);
    t.gridLineColor = {0.88f, 0.88f, 0.88f, 1};
    t.lineWidth = 2.0f;
    t.barCornerRadius = 4.0f;
    return t;
}

NVTheme NVTheme::solarized() {
    NVTheme t;
    t.name = "Solarized";
    t.backgroundColor = colorHex(0xFDF6E3);
    t.plotAreaColor = colorHex(0xFDF6E3);
    t.seriesColors = {
        colorHex(0x268BD2), colorHex(0xD33682), colorHex(0x859900),
        colorHex(0xB58900), colorHex(0x2AA198), colorHex(0xCB4B16),
        colorHex(0x6C71C4), colorHex(0xDC322F)
    };
    t.axisLineColor = colorHex(0x93A1A1);
    t.axisLabelColor = colorHex(0x657B83);
    t.gridLineColor = colorHex(0xEEE8D5);
    t.titleColor = colorHex(0x073642);
    return t;
}

NVTheme NVTheme::dark() {
    NVTheme t;
    t.name = "Dark";
    t.backgroundColor = colorHex(0x1E1E1E);
    t.plotAreaColor = colorHex(0x252525);
    t.seriesColors = {
        colorHex(0x4FC3F7), colorHex(0xFF8A65), colorHex(0x81C784),
        colorHex(0xFFD54F), colorHex(0xBA68C8), colorHex(0x4DD0E1),
        colorHex(0xF06292), colorHex(0xAED581), colorHex(0xFF8A65)
    };
    t.axisLineColor = {0.5f, 0.5f, 0.5f, 1};
    t.axisLabelColor = {0.7f, 0.7f, 0.7f, 1};
    t.gridLineColor = {0.25f, 0.25f, 0.25f, 1};
    t.titleColor = {0.9f, 0.9f, 0.9f, 1};
    t.legendBorderColor = {0.35f, 0.35f, 0.35f, 1};
    t.legendBackgroundColor = {0.15f, 0.15f, 0.15f, 0.95f};
    return t;
}

NVTheme NVTheme::monochrome() {
    NVTheme t;
    t.name = "Monochrome";
    t.seriesColors = {
        colorHex(0x333333), colorHex(0x666666), colorHex(0x999999),
        colorHex(0xBBBBBB), colorHex(0x444444), colorHex(0x777777),
        colorHex(0xAAAAAA), colorHex(0xDDDDDD)
    };
    t.gridLineColor = {0.92f, 0.92f, 0.92f, 1};
    return t;
}

NVTheme NVTheme::pastel() {
    NVTheme t;
    t.name = "Pastel";
    t.seriesColors = {
        colorHex(0x8DD3C7), colorHex(0xFDB462), colorHex(0xB3DE69),
        colorHex(0xFCCDE5), colorHex(0xBEBADA), colorHex(0xFB8072),
        colorHex(0x80B1D3), colorHex(0xD9D9D9), colorHex(0xBC80BD)
    };
    t.plotAreaColor = colorHex(0xFAFAFA);
    t.lineWidth = 2.5f;
    t.barCornerRadius = 6.0f;
    return t;
}

} // namespace nv
