#include "controller/Controller.h"
#include "view/TextualView.h"
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include <vector>

int main() {
    SQLiteScheduleDatabase db("events.db");
    std::vector<Event> initial;
    Model model(initial, &db);
    TextualView view(model);
    Controller controller(model, view);
    controller.run();
    return 0;
}
