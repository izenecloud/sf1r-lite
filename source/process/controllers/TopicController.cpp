#include "TopicController.h"

#include <common/Keys.h>

#include <bundles/mining/MiningSearchService.h>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

/**
 * @brief Action @b get_similar. Get similar topics from one topic id
 *
 * @section request
 *
 * - @b collection* (@c String): Find similar topics in this collection.
 * - @b similar_to* (@c Object): Specify the target topic id.
 *   - @c {"similar_to":{"id":1}} get topics' names similar to the one with tid=1
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a topic.
 *   - @b name (@c String): The name of topic
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "similar_to" => {
 *     "id": "1",
 *   }
 * }
 * @endcode
 */
void TopicController::get_similar()
{
//   std::cout<<"TopicController::get_similar"<<std::endl;
    IZENELIB_DRIVER_BEFORE_HOOK(requireTID_());
    std::vector<izenelib::util::UString> label_list;
    MiningSearchService* service = collectionHandler_->miningSearchService_;	
    bool requestSent = service->getSimilarLabelStringList(tid_, label_list);

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
//   std::cout<<"find similar : "<<label_list.size()<<std::endl;
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for (uint32_t i=0;i<label_list.size();i++)
    {
        Value& new_resource = resources();
        std::string str;
        label_list[i].convertString(str, izenelib::util::UString::UTF_8);
        new_resource[Keys::name] = str;

    }
}

bool TopicController::requireTID_()
{
    if (!izenelib::driver::nullValue( request()[Keys::similar_to] ) )
    {
        Value& similar_to = request()[Keys::similar_to];
        tid_ = asUint(similar_to[Keys::id]);
    }
    else
    {
        response().addError("Require field similar_to in request.");
        return false;
    }

    return true;
}

}
