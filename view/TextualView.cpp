#include "TextualView.h"
#include <format>

TextualView::TextualView(const ReadOnlyModel &model)
    : View(model)
{
}

TextualView::~TextualView() = default;

void TextualView::render()
{
    // Fetch “everything” by using a far‑future cutoff.
    auto farFuture = std::chrono::system_clock::now() + std::chrono::hours(24 * 365);
    auto events = model_.getEvents(-1, farFuture);
    renderEvents(events);
}

void TextualView::renderEvents(const std::vector<Event> &events)
{
    if (events.empty())
    {
        std::cout << "(no scheduled events)\n";
        return;
    }

    for (const auto &e : events)
    {
        auto z = std::chrono::current_zone();
        std::chrono::zoned_time zt{z, e.getTime()};
        auto timeStr = std::format("{:%Y-%m-%d %H:%M}", zt);

        std::cout << "[" << e.getId() << "] \""
                  << e.getTitle() << "\" @ " << timeStr;
        if (e.isRecurring())
            std::cout << " (recurring)";
        std::cout << "\n";
    }
}

