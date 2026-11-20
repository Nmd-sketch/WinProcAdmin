#include "../../include/gui/gui.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {
constexpr Color kHeaderColor{32, 38, 48, 255};
constexpr Color kPanelColor{248, 249, 251, 255};
constexpr Color kHeaderRowColor{232, 237, 244, 255};
constexpr Color kSelectedRowColor{205, 224, 248, 255};
constexpr Color kAlternateRowColor{241, 244, 248, 255};
constexpr Color kGridColor{220, 225, 233, 255};
constexpr Color kMutedTextColor{120, 132, 148, 255};
constexpr Color kScrollTrackColor{231, 235, 240, 255};
constexpr Color kScrollThumbColor{168, 178, 192, 255};
constexpr Color kScrollThumbHoverColor{136, 150, 168, 255};
constexpr Color kElevatedUsageColor{255, 193, 7, 255};
constexpr Color kHighUsageColor{220, 53, 69, 255};

constexpr float kRowHeight = 34.0f;
constexpr int kCellFontSize = 16;
constexpr float kHeaderHeight = 30.0f;
constexpr float kColumnPadding = 12.0f;
constexpr float kMinColumnWidth = 90.0f;
constexpr float kColumnResizeHitArea = 5.0f;
constexpr float kScrollBarWidth = 24.0f;
constexpr float kScrollBarGap = 4.0f;
constexpr float kMinThumbHeight = 24.0f;
constexpr float kHScrollBarHeight = 24.0f;
constexpr float kMinContentWidth = 1500.0f;
constexpr int kProcessDividerCount = static_cast<int>(kProcessColumnCount) - 1;

constexpr Vector2 kGuiTopMenuCoords = Vector2{0, 0};
constexpr float kGuiTopMenuHeight = 24.0f;
constexpr float kTopBarHeight = 52.0f;
constexpr float kTabBarY = kGuiTopMenuHeight + kTopBarHeight + 8.0f;
constexpr float kContentY = kTabBarY + 36.0f;
constexpr float kMenuButtonWidth = 62.0f;
constexpr float kMenuPopupWidth = 148.0f;
constexpr float kMenuOptionHeight = 24.0f;
}

namespace {
const char* const kProcessColumnLabels[kProcessColumnCount]{
    "Name", "PID", "Threads", "CPU", "Disk", "Memory",
};

std::string formatDiskUsage(double bytesPerSecond)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(1);
    if (bytesPerSecond >= 1024.0 * 1024.0) {
        out << bytesPerSecond / (1024.0 * 1024.0) << " MB/s";
    } else if (bytesPerSecond >= 1024.0) {
        out << bytesPerSecond / 1024.0 << " KB/s";
    } else {
        out << bytesPerSecond << " B/s";
    }
    return out.str();
}

std::string processCellText(const ProcessData& entry, std::size_t column)
{
    if (!entry.metricsAvailable && column >= 3) {
        return "N/A";
    }
    switch (column) {
        case 0: return std::string(entry.processName.data());
        case 1: return std::to_string(entry.processId);
        case 2: return std::to_string(entry.numberOfThreads);
        case 3: return std::to_string(entry.cpuUsage) + "%";
        case 4: return formatDiskUsage(entry.diskUsage);
        case 5: return std::to_string(entry.memWorkingSet) + " MB";
        default: return {};
    }
}

enum class UsageLevel { None, Elevated, High };

UsageLevel usageLevelFor(const ProcessData& entry, std::size_t column)
{
    if (!entry.metricsAvailable) {
        return UsageLevel::None;
    }

    switch (column) {
        case 3:
            if (entry.cpuUsage >= 50) return UsageLevel::High;
            if (entry.cpuUsage >= 20) return UsageLevel::Elevated;
            return UsageLevel::None;
        case 4:
            if (entry.diskUsage >= 10.0f * 1024.0f * 1024.0f) return UsageLevel::High;
            if (entry.diskUsage >= 1024.0f * 1024.0f) return UsageLevel::Elevated;
            return UsageLevel::None;
        case 5:
            if (entry.memWorkingSet >= 1000) return UsageLevel::High;
            if (entry.memWorkingSet >= 400) return UsageLevel::Elevated;
            return UsageLevel::None;
        default:
            return UsageLevel::None;
    }
}
}

Gui::Gui(int width, int height) : screenWidth(width), screenHeight(height)
{
    pollFn = [] {};
    refreshFn = [] {};
    selectionFn = [](int) {};
    tabChangeFn = [](int) {};
}

void Gui::setPollCallback(PollCallback callback) { pollFn = std::move(callback); }
void Gui::setRefreshCallback(RefreshCallback callback) { refreshFn = std::move(callback); }
void Gui::setSelectionCallback(SelectionCallback callback) { selectionFn = std::move(callback); }
void Gui::setTabChangeCallback(TabChangeCallback callback) { tabChangeFn = std::move(callback); }

void Gui::setProcessQueue(ProcessQueue* q) {
    m_pQueue = q;
}

void Gui::startGui() noexcept
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "WinProcAdmin");
    if (!IsWindowReady()) {
        return;
    }

    SetTargetFPS(60);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);

    while (!WindowShouldClose()) {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        pollFn();
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        BeginDrawing();
        ClearBackground(kPanelColor);

        drawTopMenu();
        drawTopBar();
        drawTabs();

        const Rectangle content{
            12.0f, kContentY,
            std::max(80.0f, static_cast<float>(screenWidth) - 24.0f),
            std::max(80.0f, static_cast<float>(screenHeight) - kContentY - 38.0f),
        };

        if (activeTab == 0) {
            drawProcessesTab(content);
        } else {
            drawPlaceholderTab(content);
        }

        drawStatusBar();
        drawTopMenuPopup();
        EndDrawing();
    }

    CloseWindow();

}

namespace {
const char* const kTopMenuLabels[]{"Files", "Options", "View"};
const char* const kTopMenuOptions[][4]{
    {"New task", "Open task", "Save list", "Exit"},
    {"Settings", "Preferences", "Reset defaults", nullptr},
    {"Always on top", "Show CPU", "Show memory", nullptr},
};
constexpr int kTopMenuOptionCounts[]{4, 3, 3};
}

void Gui::drawTopMenu()
{
    const float width = static_cast<float>(screenWidth);
    DrawRectangle(kGuiTopMenuCoords.x, kGuiTopMenuCoords.y, width, kGuiTopMenuHeight, kHeaderColor);
    DrawLine(0, static_cast<int>(kGuiTopMenuHeight), static_cast<int>(width),
             static_cast<int>(kGuiTopMenuHeight), kGridColor);

    const int previousTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 12);

    float x = kGuiTopMenuCoords.x + 4.0f;
    for (int i = 0; i < 3; ++i) {
        const Rectangle button{x, kGuiTopMenuCoords.y, kMenuButtonWidth, kGuiTopMenuHeight};
        if (GuiButton(button, kTopMenuLabels[i])) {
            openTopMenu = (openTopMenu == i) ? -1 : i;
        }
        x += kMenuButtonWidth + 2.0f;
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, previousTextSize);
}

void Gui::drawTopMenuPopup()
{
    if (openTopMenu < 0) {
        return;
    }

    const float x = kGuiTopMenuCoords.x + 4.0f
                  + static_cast<float>(openTopMenu) * (kMenuButtonWidth + 2.0f);
    const float y = kGuiTopMenuCoords.y + kGuiTopMenuHeight;
    const int count = kTopMenuOptionCounts[openTopMenu];
    const Rectangle panel{x, y, kMenuPopupWidth, static_cast<float>(count) * kMenuOptionHeight + 4.0f};

    GuiPanel(panel, nullptr);

    const int previousTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
    for (int i = 0; i < count; ++i) {
        const Rectangle option{x + 2.0f, y + 2.0f + static_cast<float>(i) * kMenuOptionHeight,
                               kMenuPopupWidth - 4.0f, kMenuOptionHeight};
        if (GuiButton(option, kTopMenuOptions[openTopMenu][i])) {
            openTopMenu = -1;
        }
    }
    GuiSetStyle(DEFAULT, TEXT_SIZE, previousTextSize);

    const Vector2 mouse = GetMousePosition();
    const Rectangle bar{kGuiTopMenuCoords.x, kGuiTopMenuCoords.y,
                        static_cast<float>(screenWidth), kGuiTopMenuHeight};
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        !CheckCollisionPointRec(mouse, bar) && !CheckCollisionPointRec(mouse, panel)) {
        openTopMenu = -1;
    }
}

void Gui::drawTopBar()
{
    const float width = static_cast<float>(screenWidth);
    const float y = kGuiTopMenuHeight;
    DrawRectangle(0, y, width, kTopBarHeight, kHeaderColor);
    DrawText("Task Manager", 16, static_cast<int>(y) + 15, 20, RAYWHITE);
    DrawText("WinProcAdmin", 168, static_cast<int>(y) + 19, 13, kMutedTextColor);

    if (GuiButton({width - 96.0f, y + 12.0f, 80.0f, 28.0f}, "Refresh")) {
        refreshFn();
    }
}

void Gui::drawTabs()
{
    const int previousTab = activeTab;
    GuiTabBar(
        {12.0f, kTabBarY, static_cast<float>(screenWidth) - 24.0f, 28.0f},
        "Processes;Performance;Users;Details;Services",
        &tabScroll,
        &activeTab);

    if (activeTab != previousTab) {
        tableScroll = 0;
        columnScroll = 0;
        tabChangeFn(activeTab);
    }
}

void Gui::drawProcessesTab(Rectangle bounds)
{
    if (m_pQueue != nullptr) {
        processes = m_pQueue->drain();
    }

    DrawRectangleRec(bounds, kPanelColor);
    DrawRectangleLinesEx(bounds, 1.0f, kGridColor);

    const float viewWidth = bounds.width - 2.0f - kScrollBarWidth - kScrollBarGap;
    const float contentWidth = std::max(viewWidth, kMinContentWidth);
    const float rowsBottom = bounds.y + bounds.height - kHScrollBarHeight - kScrollBarGap - 2.0f;
    const Rectangle header{bounds.x + 1.0f, bounds.y + 1.0f, viewWidth, kHeaderHeight};
    const float rowsY = header.y + header.height;
    const float rowsHeight = std::max(0.0f, rowsBottom - rowsY);

    const int maxVisibleRows = std::max(1, static_cast<int>(rowsHeight / kRowHeight));
    const int maxRowScroll = std::max(0, static_cast<int>(processes.procDataBuf.size()) - maxVisibleRows);
    const int maxColumnScroll = std::max(
        0, static_cast<int>(std::lround(contentWidth - viewWidth)));
    tableScroll = std::clamp(tableScroll, 0, maxRowScroll);
    columnScroll = std::clamp(columnScroll, 0, maxColumnScroll);

    const Rectangle vTrack{
        bounds.x + bounds.width - kScrollBarWidth - 2.0f,
        rowsY,
        kScrollBarWidth,
        rowsHeight,
    };
    const Rectangle hTrack{
        bounds.x + 1.0f,
        rowsBottom + kScrollBarGap,
        viewWidth,
        kHScrollBarHeight,
    };

    const Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, bounds)) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            tableScroll = std::clamp(tableScroll - static_cast<int>(wheel), 0, maxRowScroll);
        }
    }

    std::array<float, kProcessColumnCount> columnX{};
    columnX[0] = bounds.x + 1.0f;
    for (std::size_t i = 0; i + 1 < kProcessColumnCount; ++i) {
        columnX[i + 1] = columnX[i] + columnWeights[i] * contentWidth;
    }

    for (int i = 0; i < kProcessDividerCount; ++i) {
        const Rectangle grip{columnX[static_cast<std::size_t>(i) + 1] - columnScroll - kColumnResizeHitArea,
                             header.y,
                             2.0f * kColumnResizeHitArea,
                             rowsBottom - header.y};
        if (CheckCollisionPointRec(mouse, grip) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            draggingColumn = i;
            columnDragStartX = mouse.x;
            columnDragStartWeights[0] = columnWeights[static_cast<std::size_t>(i)];
            columnDragStartWeights[1] = columnWeights[static_cast<std::size_t>(i) + 1];
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        draggingColumn = -1;
    }

    if (draggingColumn >= 0 && contentWidth > 0.0f) {
        const float fraction = std::min(0.2f, kMinColumnWidth / contentWidth);
        const float delta = (mouse.x - columnDragStartX) / contentWidth;
        float left = columnDragStartWeights[0] + delta;
        float right = columnDragStartWeights[1] - delta;
        if (left < fraction) {
            right += fraction - left;
            left = fraction;
        }
        if (right < fraction) {
            left += fraction - right;
            right = fraction;
        }
        columnWeights[static_cast<std::size_t>(draggingColumn)] = left;
        columnWeights[static_cast<std::size_t>(draggingColumn) + 1] = right;

        columnX[0] = bounds.x + 1.0f;
        for (std::size_t i = 0; i + 1 < kProcessColumnCount; ++i) {
            columnX[i + 1] = columnX[i] + columnWeights[i] * contentWidth;
        }
    }

    bool dividerHovered = false;
    for (int i = 0; i < kProcessDividerCount; ++i) {
        const Rectangle grip{columnX[static_cast<std::size_t>(i) + 1] - columnScroll - kColumnResizeHitArea,
                             header.y,
                             2.0f * kColumnResizeHitArea,
                             rowsBottom - header.y};
        dividerHovered = dividerHovered || CheckCollisionPointRec(mouse, grip);
    }
    if (dividerHovered || draggingColumn >= 0) {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
    }

    const Rectangle clip{bounds.x + 1.0f, header.y, viewWidth, rowsBottom - header.y};
    BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                     static_cast<int>(clip.width), static_cast<int>(clip.height));

    DrawRectangleRec(header, kHeaderRowColor);
    for (std::size_t i = 0; i < kProcessColumnCount; ++i) {
        DrawText(kProcessColumnLabels[i],
                 static_cast<int>(columnX[i] + kColumnPadding - columnScroll),
                 header.y + 7, kCellFontSize, DARKGRAY);
    }

    const Rectangle view{bounds.x + 1.0f, rowsY, viewWidth, rowsHeight};
    const int lastRow = std::min(static_cast<int>(processes.procDataBuf.size()), tableScroll + maxVisibleRows);
    for (int i = tableScroll; i < lastRow; ++i) {
        const Rectangle row{
            bounds.x + 1.0f,
            rowsY + static_cast<float>(i - tableScroll) * kRowHeight,
            contentWidth,
            kRowHeight,
        };

        if (i == selectedRow) {
            DrawRectangleRec(row, kSelectedRowColor);
        } else if (i % 2 != 0) {
            DrawRectangleRec(row, kAlternateRowColor);
        }

        const ProcessData& entry = processes.procDataBuf[static_cast<std::size_t>(i)];
        const int textY = static_cast<int>(row.y + (kRowHeight - kCellFontSize) / 2.0f);
        for (std::size_t column = 0; column < kProcessColumnCount; ++column) {
            const float cellX = columnX[column] - columnScroll;
            const UsageLevel level = usageLevelFor(entry, column);
            if (level != UsageLevel::None) {
                const Color highlight = level == UsageLevel::High
                    ? kHighUsageColor : kElevatedUsageColor;
                DrawRectangleLinesEx(
                    {cellX + 3.0f, row.y + 3.0f,
                     columnWeights[column] * contentWidth - 6.0f, kRowHeight - 6.0f},
                    2.0f, highlight);
            }
            DrawText(processCellText(entry, column).c_str(),
                     static_cast<int>(cellX + kColumnPadding),
                     textY, kCellFontSize, DARKGRAY);
        }

        DrawLine(static_cast<int>(row.x), static_cast<int>(row.y + row.height),
                 static_cast<int>(row.x + contentWidth), static_cast<int>(row.y + row.height), kGridColor);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && draggingColumn < 0 &&
            CheckCollisionPointRec(mouse, view) && CheckCollisionPointRec(mouse, row)) {
            selectedRow = i;
            selectionFn(i);
        }
    }

    for (int i = 0; i < kProcessDividerCount; ++i) {
        const float x = columnX[static_cast<std::size_t>(i) + 1] - columnScroll;
        DrawLine(static_cast<int>(x), static_cast<int>(header.y),
                 static_cast<int>(x), static_cast<int>(rowsBottom), kGridColor);
        DrawRectangle(static_cast<int>(x) - 1, static_cast<int>(header.y), 2,
                      static_cast<int>(kHeaderHeight),
                      draggingColumn == i ? kScrollThumbHoverColor : kScrollThumbColor);
    }

    EndScissorMode();

    drawProcessScrollBar(vTrack, maxRowScroll, maxVisibleRows);
    drawProcessHScrollBar(hTrack, maxColumnScroll, static_cast<int>(viewWidth));
}

void Gui::drawProcessScrollBar(Rectangle track, int maxScroll, int visibleRows)
{
    DrawRectangleRec(track, kScrollTrackColor);

    const int totalRows = static_cast<int>(processes.procDataBuf.size());
    const float thumbHeight = maxScroll > 0
        ? std::max(kMinThumbHeight, track.height * static_cast<float>(visibleRows) / totalRows)
        : track.height;
    const float travel = std::max(0.0f, track.height - thumbHeight);
    const float thumbY = track.y + (maxScroll > 0
        ? travel * static_cast<float>(tableScroll) / static_cast<float>(maxScroll)
        : 0.0f);
    const Rectangle thumb{track.x, thumbY, track.width, thumbHeight};

    const Vector2 mouse = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mouse, thumb);
    DrawRectangleRec(thumb, (hovered || scrollDragging) ? kScrollThumbHoverColor : kScrollThumbColor);

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        scrollDragging = false;
    }

    if (maxScroll <= 0) {
        return;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, thumb)) {
        scrollDragging = true;
        scrollGrabOffset = static_cast<int>(mouse.y - thumb.y);
    } else if (scrollDragging) {
        const float wantedY = mouse.y - static_cast<float>(scrollGrabOffset);
        const float ratio = travel > 0.0f ? (wantedY - track.y) / travel : 0.0f;
        tableScroll = std::clamp(static_cast<int>(std::lround(ratio * maxScroll)), 0, maxScroll);
    } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, track)) {
        const int step = std::max(1, visibleRows - 1);
        if (mouse.y < thumb.y) {
            tableScroll = std::clamp(tableScroll - step, 0, maxScroll);
        } else if (mouse.y > thumb.y + thumb.height) {
            tableScroll = std::clamp(tableScroll + step, 0, maxScroll);
        }
    }
}

void Gui::drawProcessHScrollBar(Rectangle track, int maxScroll, int viewWidth)
{
    DrawRectangleRec(track, kScrollTrackColor);

    const int totalWidth = viewWidth + maxScroll;
    const float thumbWidth = maxScroll > 0
        ? std::max(kMinThumbHeight, track.width * static_cast<float>(viewWidth) / static_cast<float>(totalWidth))
        : track.width;
    const float travel = std::max(0.0f, track.width - thumbWidth);
    const float thumbX = track.x + (maxScroll > 0
        ? travel * static_cast<float>(columnScroll) / static_cast<float>(maxScroll)
        : 0.0f);
    const Rectangle thumb{thumbX, track.y, thumbWidth, track.height};

    const Vector2 mouse = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mouse, thumb);
    DrawRectangleRec(thumb, (hovered || hScrollDragging) ? kScrollThumbHoverColor : kScrollThumbColor);

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        hScrollDragging = false;
    }

    if (maxScroll <= 0) {
        return;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, thumb)) {
        hScrollDragging = true;
        hScrollGrabOffset = static_cast<int>(mouse.x - thumb.x);
    } else if (hScrollDragging) {
        const float wantedX = mouse.x - static_cast<float>(hScrollGrabOffset);
        const float ratio = travel > 0.0f ? (wantedX - track.x) / travel : 0.0f;
        columnScroll = std::clamp(static_cast<int>(std::lround(ratio * maxScroll)), 0, maxScroll);
    } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, track)) {
        const int step = std::max(1, viewWidth / 2);
        if (mouse.x < thumb.x) {
            columnScroll = std::clamp(columnScroll - step, 0, maxScroll);
        } else if (mouse.x > thumb.x + thumb.width) {
            columnScroll = std::clamp(columnScroll + step, 0, maxScroll);
        }
    }
}

void Gui::drawPlaceholderTab(Rectangle bounds) const
{
    DrawRectangleRec(bounds, kPanelColor);
    DrawRectangleLinesEx(bounds, 1.0f, kGridColor);

    const char* message = "This tab is a placeholder.";
    const int textWidth = MeasureText(message, 20);
    DrawText(message, static_cast<int>(bounds.x + (bounds.width - textWidth) / 2.0f),
             static_cast<int>(bounds.y + bounds.height / 2.0f), 20, GRAY);
}

void Gui::drawStatusBar() const
{
    std::string status = "Processes: " + std::to_string(processes.procDataBuf.size());
    if (!processes.procDataBuf.empty() && selectedRow >= 0 && selectedRow < static_cast<int>(processes.procDataBuf.size())) {
        status += "    Selected: " + std::string(processes.procDataBuf[static_cast<std::size_t>(selectedRow)].processName.data())
                + " (PID " + std::to_string(processes.procDataBuf[static_cast<std::size_t>(selectedRow)].processId) + ")";
    }

    GuiStatusBar(
        {0.0f, static_cast<float>(screenHeight) - 30.0f, static_cast<float>(screenWidth), 30.0f},
        status.c_str());
}
