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
    void drawProcessPopupMenu(Vector2 point);
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
    
    /* Screen State */
    int screenWidth;
    int screenHeight;

    /* Tabs state */
    int activeTab = 0;

    /* Scroll state */
    int tabScroll = 0;
    int tableScroll = 0;
    bool scrollDragging = false;
    int scrollGrabOffset = 0;
    int columnScroll = 0;
    bool hScrollDragging = false;
    int hScrollGrabOffset = 0;
    std::array<float, kProcessColumnCount> columnWeights{0.30f, 0.12f, 0.13f, 0.15f, 0.15f, 0.15f};
    int draggingColumn = -1;
    float columnDragStartX = 0.0f;
    std::array<float, kProcessColumnCount - 1> columnDragStartWeights{};

    /* Current row selection */
    int selectedRow = 0;
    
    /* Popup menu state */
    int openTopMenu = -1;
    int openProcessPopupMenu = -1;
    bool procCoordsSet = false;
    bool isProcessSelected = false;
    ProcessData pSelected;
    Vector2 processPopupMenuCoords = {};
    
    /* Cursor state */
    enum class CursorType {
        NORMAL,
        DIVIDER
    };
    CursorType currentCursorType = CursorType::NORMAL;
    bool isCursorOverDivider = false;

    /* Process queue */
    ProcessQueue* m_pQueue = nullptr;
};
