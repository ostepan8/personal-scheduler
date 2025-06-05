// main.cpp
#include "model/Model.h"
#include "view/TextualView.h"
#include "controller/Controller.h"
#include <vector>

int main()
{
    // 1) Construct an initially empty Model.
    vector<Event> initialEvents;
    Model model(initialEvents);

    // 2) Construct a TextualView, passing the Model as ReadOnlyModel.
    TextualView view(model);

    // 3) Construct the Controller with both Model and View.
    Controller controller(model, view);

    // 4) Run the controller’s CLI loop.
    controller.run();

    return 0;
}
