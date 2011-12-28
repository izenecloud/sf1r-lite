#include "DriverLogServerController.h"


namespace sf1r
{

/**
 * @brief Action \b update_cclog
 * The request data of \b update_cclog in json format is expected supported by any action of SF1R controllers,
 * and the \b header field is presented.
 *
 * @example
 * {
 *   "header" : {
 *     "action" : "documents",
 *     "controller" : "visit"
 *   },
 *
 *   # Following by other fields (this is comment)
 * }
 *
 */
void DriverLogServerController::update_cclog()
{
    std::cout<<request().controller()<<"/"<<request().action()<<std::endl;

    // process
    std::string collection = asString(request()["collection"]);
    if (!skipProcess(collection))
    {
        ProcessorPtr processor = ProcessorFactory::get()->getProcessor(
                request().controller(), request().action());

        if (processor)
            processor->process(request());
    }

    // output TODO
    std::string raw;
    jsonWriter_.write(request().get(), raw);
    std::cout<<raw<<std::endl;
}

bool DriverLogServerController::skipProcess(const std::string& collection)
{
    if (driverCollections_.find(collection) == driverCollections_.end())
    {
        std::cout<<"skip "<<collection<<std::endl;
        return true;
    }

    return false;
}

}
