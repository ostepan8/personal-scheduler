#include "TextualView.h"
#include <iomanip>

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

    if (events.empty())
    {
        std::cout << "(no scheduled events)\n";
        return;
    }

    for (const auto &e : events)
    {
        auto tp = e.getTime();
        std::time_t t_c = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_buf;
#if defined(_MSC_VER)
        localtime_s(&tm_buf, &t_c);
#else
        localtime_r(&t_c, &tm_buf);
#endif
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);

        std::cout << "[" << e.getId() << "] \""
                  << e.getTitle() << "\" @ " << buf;
        if (e.isRecurring())
            std::cout << " (recurring)";
        std::cout << "\n";
    }
}

