#pragma once
#include "httplib.h"
#include "../../model/Model.h"

namespace EventRoutes {
    void registerRoutes(httplib::Server &server, Model &model);
}
