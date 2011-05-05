#include "OrderManager.h"

namespace sf1r
{

OrderManager::OrderManager(const std::string& path)
    :item_order_index_(path+"/index")
{
}

void OrderManager::addOrder(
	sf1r::orderid_t orderId,
	std::list<sf1r::itemid_t>& items
)
{
    item_order_index_.add(orderId, items);
}

void OrderManager::flush()
{
    item_order_index_.flush();
}


}
