#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include "../ProcessData.hpp"
#include "../ProcessQueue.hpp"

#include <raylib.h>
#include <raygui.h>

constexpr std::size_t kProcessColumnCount = 6;

class Gui {
public:
    using PollCallback = std::function<void()>;
    using RefreshCallback = std::function<void()>;
    using SelectionCallback = std::function<void(int)>;
    using TabChangeCallback = std::function<void(int)>;

    Gui(int width, int height);

    void startGui() noexcept;

    void setPollCallback(PollCallback callback);
    void setRefreshCallback(RefreshCallback callback);
    void setSelectionCallback(SelectionCallback callback);
    void setTabChangeCallback(TabChangeCallback callback);
    void setProcessQueue(ProcessQueue* q);

private:

    void drawTopBar();
    void drawTabs();
    void drawProcessesTab(Rectangle bounds);
    void drawProcessScrollBar(Rectangle track, int maxScroll, int visibleRows);
    void drawProcessHScrollBar(Rectangle track, int maxScroll, int viewWidth);
    void drawPlaceholderTab(Rectangle bounds) const;
    void drawStatusBar() const;
    void drawTopMenu();
    void drawTopMenuPopup();
    void drawPerformanceTab(Rectangle bounds);

    ProcessSnapshot processes;
    PollCallback pollFn;
    RefreshCallback refreshFn;
    SelectionCallback selectionFn;
    TabChangeCallback tabChangeFn;

    int screenWidth;
    int screenHeight;
    int activeTab = 0;
    int tabScroll = 0;
    int selectedRow = 0;
    int tableScroll = 0;
    bool scrollDragging = false;
    int scrollGrabOffset = 0;
    int columnScroll = 0;
    bool hScrollDragging = false;
    int hScrollGrabOffset = 0;
    int openTopMenu = -1;

    std::array<float, kProcessColumnCount> columnWeights{0.30f, 0.12f, 0.13f, 0.15f, 0.15f, 0.15f};
    int draggingColumn = -1;
    float columnDragStartX = 0.0f;
    std::array<float, kProcessColumnCount - 1> columnDragStartWeights{};

    ProcessQueue* m_pQueue = nullptr;
};
