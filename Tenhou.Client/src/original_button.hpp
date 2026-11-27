#pragma once
#include <windows.h>
#include <algorithm>

namespace tenhou {
    inline constexpr wchar_t kOriginalUiFace[] = L"MS UI Gothic";

    inline HFONT CreateOriginalUiFont() {
        LOGFONTW font{};
        GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(font), &font);
        font.lfHeight = -12;
        wcscpy_s(font.lfFaceName, kOriginalUiFace);
        return CreateFontIndirectW(&font);
    }

    inline void PaintOriginalButton(const DRAWITEMSTRUCT &item, const HFONT font, const wchar_t *label) {
        // WC_ITEM_BTN (004a7920): white upper half, gray lower gradient,
        // disabled gray, text extent +16/+8.
        const auto r = item.rcItem;
        const bool enabled = IsWindowEnabled(item.hwndItem) != FALSE;
        const int width = r.right - r.left, height = r.bottom - r.top;
        for (int y = 0; y < height; ++y) {
            int value = enabled
                            ? (y < height / 2
                                   ? 255
                                   : 221 + 34 * (y - height / 2) / std::max(1, height - height / 2 - 1))
                            : 204;
            RECT line{0, y, width, y + 1};
            const auto brush = CreateSolidBrush(RGB(value, value, value));
            FillRect(item.hDC, &line, brush);
            DeleteObject(brush);
        }
        RECT text = r;
        if (item.itemState & ODS_SELECTED) OffsetRect(&text, 1, 1);
        const auto old = SelectObject(item.hDC, font);
        SetBkMode(item.hDC, TRANSPARENT);
        SetTextColor(item.hDC, enabled ? RGB(34, 34, 34) : RGB(102, 102, 102));
        DrawTextW(item.hDC, label, -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(item.hDC, old);
        POINT point{};
        GetCursorPos(&point);
        ScreenToClient(item.hwndItem, &point);
        const auto border = CreateSolidBrush(enabled && PtInRect(&r, point) ? RGB(51, 153, 255) : RGB(136, 136, 136));
        FrameRect(item.hDC, &r, border);
        DeleteObject(border);
        if (item.itemState & ODS_FOCUS) {
            InflateRect(&text, -2, -2);
            DrawFocusRect(item.hDC, &text);
        }
    }
}
