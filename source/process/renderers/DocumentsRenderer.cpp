/**

 * @file core/common/renderers/DocumentsRenderer.cpp
 * @author Ian Yang
 * @date Created <2010-06-11 13:03:57>
 */
#include "DocumentsRenderer.h"
#include "SubDocPropValueRenderer.h"
#include <common/Keys.h>
#include <query-manager/ActionItem.h>
#include <glog/logging.h>

#include <boost/assert.hpp>

namespace
{
const izenelib::util::UString::EncodingType kEncoding =
    izenelib::util::UString::UTF_8;
}

namespace sf1r {

using namespace izenelib::driver;
using driver::Keys;

template <class DocumentResultsType>
void renderPropertyList(
    SplitPropValueRenderer& splitRenderer,
    const std::vector<DisplayProperty>& propertyList,
    const DocumentResultsType& docResults,
    std::size_t column,
    Value& newResource
)
{
    // full text and snippet properties
    std::size_t summaryIndex = 0;
    std::string propertyValueBuffer;

    for (std::size_t p = 0; p < propertyList.size(); ++p)
    {
        const std::string& propertyName = propertyList[p].propertyString_;

        const PropertyValue::PropertyValueStrType& snippetText =
            docResults.snippetTextOfDocumentInPage_[p][column];
            
        if (propertyList[p].isSplitPropertyValue_)
        {
            splitRenderer.renderPropValue(
                propertyName, snippetText, newResource[propertyName]
            );
        }
        else if (propertyList[p].isSubDocPropertyValue_)
        {
            propertyValueBuffer = propstr_to_str(snippetText, kEncoding);
            SubDocPropValueRenderer::renderSubDocPropValue(
                propertyName, propertyValueBuffer, newResource[propertyName]
            );
        }
        else
        {
            propertyValueBuffer = propstr_to_str(snippetText, kEncoding);
            // remove dummy token @@ALL@@ from result
            if (propertyName == "ACL_ALLOW" && propertyValueBuffer == "@@ALL@@")
            {
                propertyValueBuffer = "";
            }

            newResource[propertyName] = propertyValueBuffer;
        }

        if (propertyList[p].isSummaryOn_)
        {
            BOOST_ASSERT(summaryIndex < docResults.rawTextOfSummaryInPage_.size());

            propertyValueBuffer = propstr_to_str(docResults.rawTextOfSummaryInPage_[summaryIndex][column], kEncoding);

            const std::string& summaryPropertyName =
                propertyList[p].summaryPropertyAlias_;

            newResource[summaryPropertyName] = propertyValueBuffer;
            ++summaryIndex;
        }
    }
}

DocumentsRenderer::DocumentsRenderer(const MiningSchema& miningSchema, int topKNum)
    : splitRenderer_(miningSchema)
    , TOP_K_NUM(topKNum)
{
}

/**
 * @brief Render documents in response
 *
 * Dummy token @@ALL@@ is removed from ACL_ALLOW.
 */
void DocumentsRenderer::renderDocuments(
    const std::vector<DisplayProperty>& propertyList,
    const RawTextResultFromMIA& result,
    izenelib::driver::Value& resources
)
{
    std::vector<sf1r::wdocid_t> widList;
    result.getWIdList(widList);

    std::size_t resultCount = widList.size();

    for (std::size_t i = 0; i < resultCount; ++i)
    {
        Value& newResource = resources();

        newResource[Keys::_id] = widList[i];

        renderPropertyList(splitRenderer_, propertyList,
            result, i, newResource);

        if (result.numberOfDuplicatedDocs_.size()
            == widList.size())
        {
            newResource[Keys::_duplicated_document_count] =
                result.numberOfDuplicatedDocs_[i];
        }

//         if (result.topKtids_.size()
//             == widList.size())
//         {
//             newResource[Keys::_tid] =
//                 result.topKtids_[i];
//         }
    }
}

/**
 * @brief Render documents in response
 *
 * Dummy token @@ALL@@ is removed from ACL_ALLOW.
 */
void DocumentsRenderer::renderDocuments(
    const std::vector<DisplayProperty>& propertyList,
    const KeywordSearchResult& searchResult,
    izenelib::driver::Value& resources
)
{
    std::string strBuffer;

    std::size_t indexInTopK = searchResult.start_ % TOP_K_NUM;

    BOOST_ASSERT(indexInTopK + searchResult.count_ <= searchResult.topKDocs_.size());

    std::vector<sf1r::wdocid_t> topKWDocs;
    searchResult.getTopKWDocs(topKWDocs);

    for (std::size_t i = 0; i < searchResult.count_; ++i, ++indexInTopK)
    {
        Value& newResource = resources();
        newResource[Keys::_id] = topKWDocs[indexInTopK];
        newResource[Keys::_rank] = searchResult.topKRankScoreList_[indexInTopK];

        if (searchResult.topKCustomRankScoreList_.size()
                == searchResult.topKDocs_.size())
        {
            newResource[Keys::_custom_rank] = searchResult.topKCustomRankScoreList_[indexInTopK];
        }

        renderPropertyList(splitRenderer_, propertyList,
            searchResult, i, newResource);

        if (searchResult.numberOfDuplicatedDocs_.size()
            == searchResult.topKDocs_.size())
        {
            newResource[Keys::_duplicated_document_count] =
                searchResult.numberOfDuplicatedDocs_[indexInTopK];
        }

        if (searchResult.numberOfSimilarDocs_.size()
            == searchResult.topKDocs_.size())
        {
            newResource[Keys::_similar_document_count] =
                searchResult.numberOfSimilarDocs_[indexInTopK];
        }

        if (searchResult.topKtids_.size()
            == searchResult.topKDocs_.size())
        {
            newResource[Keys::_tid] =
                searchResult.topKtids_[indexInTopK];
        }

        if (searchResult.imgs_.size() == searchResult.topKDocs_.size())
        {
            newResource[Keys::_image_id] = searchResult.imgs_[indexInTopK];
        }

        if (searchResult.docCategories_.size()
            == searchResult.topKDocs_.size())
        {
            Value& documentCategories = newResource[Keys::_categories];
            std::string categoryValueBuffer;
            for(std::size_t c = 0;
                c < searchResult.docCategories_[indexInTopK].size(); ++c)
            {
                documentCategories() = propstr_to_str(searchResult.docCategories_[indexInTopK][c], kEncoding);
            }
        }
    }
}

void DocumentsRenderer::renderRelatedQueries(
    const KeywordSearchResult& miaResult,
    izenelib::driver::Value& relatedQueries
)
{
    std::string tmpstr;
    for (std::size_t i = 0; i < miaResult.relatedQueryList_.size(); ++i)
    {
        miaResult.relatedQueryList_[i].convertString(tmpstr, kEncoding);
        relatedQueries() = tmpstr;
    }
}

void DocumentsRenderer::renderTaxonomy(
    const KeywordSearchResult& miaResult,
    izenelib::driver::Value& taxonomy
)
{
    if (miaResult.taxonomyString_.empty())
    {
        return;
    }

    BOOST_ASSERT(miaResult.taxonomyString_.size() == miaResult.taxonomyLevel_.size());
    BOOST_ASSERT(miaResult.taxonomyString_.size() == miaResult.numOfTGDocs_.size());
    std::vector<Value*> taxonomyParents;
    taxonomyParents.push_back(&taxonomy[Keys::labels]);
    for (std::size_t i = 0; i < miaResult.taxonomyString_.size(); ++i)
    {
        std::size_t currentLevel = miaResult.taxonomyLevel_[i];
        BOOST_ASSERT(currentLevel < taxonomyParents.size());

        Value& parent = *(taxonomyParents[currentLevel]);
        Value& newLabel = parent();
        newLabel[Keys::label] = propstr_to_str(miaResult.taxonomyString_[i], kEncoding);
        newLabel[Keys::document_count] = miaResult.numOfTGDocs_[i];

        // alternative for the parent of next level
        std::size_t nextLevel = currentLevel + 1;
        taxonomyParents.resize(nextLevel + 1);
        taxonomyParents[nextLevel] = &newLabel[Keys::sub_labels];
    }
}

void DocumentsRenderer::renderNameEntity(
    const KeywordSearchResult& miaResult,
    izenelib::driver::Value& nameEntity
)
{
    std::string tmpstr;
    for (std::size_t i = 0; i < miaResult.neList_.size(); ++i)
    {
        Value& newNameEntity = nameEntity();
        miaResult.neList_[i].type.convertString(tmpstr, kEncoding);
        newNameEntity[Keys::type] = tmpstr;

        Value& nameEntityList = newNameEntity[Keys::name_entity_list];
        typedef std::vector<NEItem>::const_iterator const_iterator;
        for (const_iterator item = miaResult.neList_[i].item_list.begin(),
                         itemEnd = miaResult.neList_[i].item_list.end();
             item != itemEnd; ++item)
        {
            Value& newNameEntityItem = nameEntityList();
            item->text.convertString(tmpstr, kEncoding);
            newNameEntityItem[Keys::name_entity_item] = tmpstr;
            newNameEntityItem[Keys::document_support_count] = item->doc_list.size();
        }
    }
}

void DocumentsRenderer::renderFaceted(
    const KeywordSearchResult& miaResult,
    izenelib::driver::Value& faceted)
{
  const std::list<sf1r::faceted::OntologyRepItem>& item_list = miaResult.onto_rep_.item_list;
  if( item_list.empty())
  {
      return;
  }

  std::vector<Value*> parents;
  parents.push_back(&faceted[Keys::labels]);
  std::list<sf1r::faceted::OntologyRepItem>::const_iterator it = item_list.begin();
  while(it!=item_list.end())
  {
      const sf1r::faceted::OntologyRepItem& item = *it;
      std::size_t currentLevel = item.level-1;
      BOOST_ASSERT(currentLevel < parents.size());

      std::string tmpstr;
      Value& parent = *(parents[currentLevel]);
      Value& newLabel = parent();
      item.text.convertString(tmpstr, kEncoding);
      newLabel[Keys::label] = tmpstr;
      newLabel[Keys::id] = item.id;
      newLabel[Keys::document_count] = item.doc_count;
#ifdef ONTOLOGY
      cout<<"currentLevel is "<<currentLevel<<endl;
      cout<<"parents.size() is "<<parents.size()<<endl;
      //cout<<"convertBuffer is "<<convertBuffer<<endl;
      cout<<"itemid is "<<item.id<<endl;
      cout<<"item.doc_count is "<<item.doc_count<<endl;
#endif
      // alternative for the parent of next level
      std::size_t nextLevel = currentLevel + 1;
      parents.resize(nextLevel + 1);
      parents[nextLevel] = &newLabel[Keys::sub_labels];
      ++it;
  }
}

void DocumentsRenderer::renderGroup(
    const KeywordSearchResult& miaResult,
    izenelib::driver::Value& groupResult
)
{
    const std::list<sf1r::faceted::OntologyRepItem>& item_list = miaResult.groupRep_.stringGroupRep_;
    if (item_list.empty())
    {
        return;
    }


    std::vector<Value*> parents;
    parents.push_back(&groupResult);
    std::string tmpstr;
    for (std::list<sf1r::faceted::OntologyRepItem>::const_iterator it = item_list.begin();
            it != item_list.end(); ++it)
    {
        const sf1r::faceted::OntologyRepItem& item = *it;
        // group level start from 0
        std::size_t currentLevel = item.level;
        BOOST_ASSERT(currentLevel < parents.size());

        Value& parent = *(parents[currentLevel]);
        Value& newLabel = parent();
        // alternative for the parent of next level
        std::size_t nextLevel = currentLevel + 1;
        parents.resize(nextLevel + 1);
        item.text.convertString(tmpstr, kEncoding);
        if (currentLevel == 0)
        {
            newLabel[Keys::property] = tmpstr;
            newLabel[Keys::document_count] = item.doc_count;
            parents[nextLevel] = &newLabel[Keys::labels];
        }
        else
        {
            newLabel[Keys::label] = tmpstr;
            newLabel[Keys::document_count] = item.doc_count;
            parents[nextLevel] = &newLabel[Keys::sub_labels];
        }
    }
}

void DocumentsRenderer::renderAttr(
    const KeywordSearchResult& miaResult,
    izenelib::driver::Value& attrResult
)
{
    const std::list<sf1r::faceted::OntologyRepItem>& item_list = miaResult.attrRep_.item_list;
    if (item_list.empty())
    {
        return;
    }

    Value* parent = NULL;
    std::string tmpstr;
    for (std::list<sf1r::faceted::OntologyRepItem>::const_iterator it = item_list.begin();
        it != item_list.end(); ++it)
    {
        const sf1r::faceted::OntologyRepItem& item = *it;

        item.text.convertString(tmpstr, kEncoding);
        // attribute name
        if (item.level == 0)
        {
            Value& newLabel = attrResult();
            newLabel[Keys::attr_name] = tmpstr;
            newLabel[Keys::document_count] = item.doc_count;
            parent = &newLabel[Keys::labels];
        }
        // attribute value
        else
        {
            BOOST_ASSERT(parent);
            Value& newLabel = (*parent)();
            newLabel[Keys::label] = tmpstr;
            newLabel[Keys::document_count] = item.doc_count;
        }
    }
}

void DocumentsRenderer::renderTopGroupLabel(const KeywordSearchResult& miaResult,
                                            izenelib::driver::Value& renderValue)
{
    using faceted::GroupParam;

    const GroupParam::GroupLabelMap& topLabels = miaResult.autoSelectGroupLabels_;
    if (topLabels.empty())
        return;

    for (GroupParam::GroupLabelMap::const_iterator labelIt = topLabels.begin();
         labelIt != topLabels.end(); ++labelIt)
    {
        Value& propValue = renderValue();
        propValue[Keys::group_property] = labelIt->first;

        Value& labelValue = propValue[Keys::group_label];
        const GroupParam::GroupPathVec& pathVec = labelIt->second;

        for (GroupParam::GroupPathVec::const_iterator pathIt = pathVec.begin();
             pathIt != pathVec.end(); ++pathIt)
        {
            Value& pathValue = labelValue();
            for (GroupParam::GroupPath::const_iterator nodeIt = pathIt->begin();
                 nodeIt != pathIt->end(); ++nodeIt)
            {
                pathValue() = *nodeIt;
            }
        }
    }
}

} // namespace sf1r
