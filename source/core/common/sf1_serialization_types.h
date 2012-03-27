#ifndef SF1_TYPES_H_
#define SF1_TYPES_H_

#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <common/type_defs.h>

#include <document-manager/DocumentManager.h>


#include <string>
#include <vector>
#include <list>
#include <map>
#include <util/ustring/UString.h>

/////////////////////////////////////////////////

#include <util/izene_serialization.h>
using namespace sf1r;

////////////////////////////////////////////////

//Febird serialization types


MAKE_FEBIRD_SERIALIZATION(std::vector<izenelib::util::UString>)
MAKE_FEBIRD_SERIALIZATION(std::vector<std::vector<izenelib::util::UString> >)
MAKE_FEBIRD_SERIALIZATION(std::vector<std::vector<unsigned int > >)
MAKE_FEBIRD_SERIALIZATION(std::vector<std::vector<std::string > >)
MAKE_FEBIRD_SERIALIZATION(std::vector<std::vector<std::vector<unsigned int > > >)
MAKE_FEBIRD_SERIALIZATION(std::vector<string_size_pair_t>)
MAKE_FEBIRD_SERIALIZATION(std::map<std::string, double>)
MAKE_FEBIRD_SERIALIZATION(std::map<std::string, std::string>)
MAKE_FEBIRD_SERIALIZATION(std::vector<uint64_t>)

MAKE_FEBIRD_SERIALIZATION(DisplayProperty)
MAKE_FEBIRD_SERIALIZATION(RequesterEnvironment)
MAKE_FEBIRD_SERIALIZATION(AutoFillActionItem)
MAKE_FEBIRD_SERIALIZATION(IndexCommandActionItem)
MAKE_FEBIRD_SERIALIZATION(PageInfo)
MAKE_FEBIRD_SERIALIZATION(LanguageAnalyzerInfo)
MAKE_FEBIRD_SERIALIZATION(KeywordSearchActionItem)
MAKE_FEBIRD_SERIALIZATION(MiningCommandActionItem)
MAKE_FEBIRD_SERIALIZATION(RecentKeywordActionItem)
MAKE_FEBIRD_SERIALIZATION(GetDocumentsByIdsActionItem)

MAKE_FEBIRD_SERIALIZATION(KeywordSearchResult)
MAKE_FEBIRD_SERIALIZATION(RawTextResultFromSIA)
MAKE_FEBIRD_SERIALIZATION(SimilarImageDocIdList)
MAKE_FEBIRD_SERIALIZATION(ErrorInfo)


MAKE_MEMCPY_SERIALIZATION(std::vector<PropertyKey>)

MAKE_FEBIRD_SERIALIZATION(std::vector<KeywordSearchActionItem::SortPriorityType>)

MAKE_FEBIRD_SERIALIZATION(std::vector<QueryFiltering::FilteringType>)

//////////////////////////////////////////////////////



#endif /*SF1_TYPES_H_*/
