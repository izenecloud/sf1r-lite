#include "b5m_helper.h"

using namespace sf1r;


const B5MPropertyList B5MHelper::B5MO_PROPERTY_LIST("B5MO_PROPERTY",boost::assign::list_of("DOCID")("DATE")("Picture")
  ("Source")("Category")("Url")("Price")
  ("Attribute")("Title")("Content")("uuid")("OriginalCategory")("OriginalPicture"));

const B5MPropertyList B5MHelper::B5MP_PROPERTY_LIST("B5MP_PROPERTY",boost::assign::list_of("DOCID")("DATE")("Picture")
  ("Source")("Category")("Url")("Price")
  ("Attribute")("Title")("Content")("itemcount")("manmade")("OriginalCategory")("OriginalPicture"));

const B5MPropertyList B5MHelper::B5MC_PROPERTY_LIST("B5MC_PROPERTY",boost::assign::list_of("DOCID")("DATE")("ComUrl")
  ("ProdDocid")("ProdName")("Source")("UserName")
  ("UsefulVoteTotal")("UsefulVote")("Content")("Integration")("Advantage")("Disadvantage")("Title")("City")("Score")("uuid"));
