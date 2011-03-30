#include "TaxonomyGenerationSubManager.h"
#include "NERRanking.hpp"
#include <mining-manager/duplicate-detection-submanager/IntergerHashFunction.h>
#include <mining-manager/util/MUtil.hpp>
#include <mining-manager/util/TermUtil.hpp>

#include <boost/timer.hpp>
#include <ir/index_manager/index/CommonItem.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <algorithm>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <cstdlib>
#include <ctype.h>
#include <la/stem/Stemmer.h>
#include <document-manager/UniqueDocIdentifier.h>
#include <cmath>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <mining-manager/util/LabelDistance.h>
#include <list>
using namespace sf1r;

TaxonomyGenerationSubManager::TaxonomyGenerationSubManager(
    const TaxonomyPara& taxonomy_param,
    const boost::shared_ptr<LabelManager>& labelManager,
    idmlib::util::IDMAnalyzer* analyzer)
        :taxonomy_param_(taxonomy_param), labelManager_(labelManager), analyzer_(analyzer)
        , algorithm_(labelManager_.get(), analyzer), tgParams_()
{
    tgParams_.maxLevel_ = taxonomy_param_.levels;
    tgParams_.perLevelNum_ = taxonomy_param_.per_levelnum;
    tgParams_.canConceptNum_ = taxonomy_param_.candidate_labelnum;
    if (tgParams_.maxLevel_ == 0 ) tgParams_.maxLevel_ = default_levels_;
    if (tgParams_.perLevelNum_ == 0) tgParams_.perLevelNum_ = default_perLevelNum_;
    if (tgParams_.canConceptNum_ == 0 ) tgParams_.canConceptNum_ = default_candLabelNum_;
    tgParams_.minDocSupp_ = 2;
    tgParams_.minContainessValue_ = 0.65;
    tgParams_.maxContainessValue_ = 0.4;
    tgParams_.specialDocCount4ContainessValue_ = 50;
    tgParams_.minContainessValue2_ = 0.6;
    tgParams_.maxContainessValue2_ = 0.55;
    tgParams_.minContainess_ = 0.19;

//     peopParams_ = tgParams_;
//     peopParams_.maxLevel_ = 1;
//     peopParams_.perLevelNum_ = miningManagerConfig_->taxonomyPara_.maxPeopNum_;
//     peopParams_.minDocSupp_ = 1;
//
//     locParams_ = tgParams_;
//     locParams_.maxLevel_ = 1;
//     locParams_.perLevelNum_ = miningManagerConfig_->taxonomyPara_.maxLocNum_;
//     locParams_.minDocSupp_ = 1;
//
//     orgParams_ = tgParams_;
//     orgParams_.maxLevel_ = 1;
//     orgParams_.perLevelNum_ = miningManagerConfig_->taxonomyPara_.maxOrgNum_;
//     orgParams_.minDocSupp_ = 1;

}

TaxonomyGenerationSubManager::~TaxonomyGenerationSubManager()
{

}


bool TaxonomyGenerationSubManager::getQuerySpecificTaxonomyInfo (
    const std::vector<docid_t>&  docIdList,
    const izenelib::util::UString& queryStr,
    uint32_t totalCount,
    uint32_t numberFromSia,
    TaxonomyRep& taxonomyRep
    ,ne_result_list_type& neList)
{
    try
    {
        if ( docIdList.size()==0 ) return true;
        bool showtime = true;
        izenelib::util::ClockTimer clocker;
        std::vector<std::vector<labelid_t> > all_label_list;
        labelManager_->getLabelsByDocIdList( docIdList, all_label_list);
        if (showtime)
        {
            std::cout<<"[TG phase 2]: "<<clocker.elapsed()<<" seconds."<<std::endl;
            clocker.restart();
        }
        if ( all_label_list.size() == 0 )
        {
            taxonomyRep.error_ = "Can not get any labels, label list is empty.";
            return false;
        }

        uint32_t tsiz = 0;
        for ( uint32_t i=0;i<all_label_list.size();i++)
        {
            tsiz += all_label_list[i].size();
        }
        if ( tsiz==0 )
        {
            //no labels to clustering
            return true;
        }
        std::vector<std::pair<labelid_t, docid_t> > inputPairList(tsiz);
        std::vector<std::pair<labelid_t, docid_t> > peopInputPairList;
        std::vector<std::pair<labelid_t, docid_t> > locInputPairList;
        std::vector<std::pair<labelid_t, docid_t> > orgInputPairList;
        uint32_t p=0;
        for ( uint32_t i=0;i<all_label_list.size();i++)
        {
            for (uint32_t j=0;j<all_label_list[i].size();j++)
            {
                uint32_t labelId = all_label_list[i][j];
                inputPairList[p].first = labelId;
                inputPairList[p].second = i;
                ++p;
                uint8_t type;
                if ( labelManager_->getLabelType(labelId, type) )
                {
                    if ( type == LabelManager::LABEL_TYPE::PEOP )
                    {
                        peopInputPairList.push_back( std::make_pair(labelId, i) );
                    }
                    else if ( type == LabelManager::LABEL_TYPE::LOC )
                    {
                        locInputPairList.push_back( std::make_pair(labelId, i) );
                    }
                    else if ( type == LabelManager::LABEL_TYPE::ORG )
                    {
                        orgInputPairList.push_back( std::make_pair(labelId, i) );
                    }
                }
            }
        }
        algorithm_.doClustering(inputPairList, queryStr, docIdList, totalCount, tgParams_, taxonomyRep.result_ );
//         std::vector<boost::shared_ptr<TgClusterRep> > nerResult;

        neList.resize(3);
        neList[0].type_ = izenelib::util::UString("PEOPLE", izenelib::util::UString::UTF_8);
        neList[1].type_ = izenelib::util::UString("LOCATION", izenelib::util::UString::UTF_8);
        neList[2].type_ = izenelib::util::UString("ORGANIZATION", izenelib::util::UString::UTF_8);

//         {
//             algorithm_.doClustering(peopInputPairList, docIdList, totalCount, peopParams_, nerResult );
//             for(uint32_t i=0;i<nerResult.size();i++)
//             {
//                 neList[0].second.push_back( nerResult[i]->name_ );
//             }
//             nerResult.clear();
//         }
//
//         {
//             algorithm_.doClustering(locInputPairList, docIdList, totalCount, locParams_, nerResult );
//             for(uint32_t i=0;i<nerResult.size();i++)
//             {
//                 neList[1].second.push_back( nerResult[i]->name_ );
//             }
//             nerResult.clear();
//         }
//
//         {
//             algorithm_.doClustering(orgInputPairList, docIdList, totalCount, orgParams_, nerResult );
//             for(uint32_t i=0;i<nerResult.size();i++)
//             {
//                 neList[2].second.push_back( nerResult[i]->name_ );
//             }
//             nerResult.clear();
//         }

        if ( taxonomy_param_.enable_nec )
        {
            getNEList_(peopInputPairList, docIdList, totalCount, taxonomy_param_.max_peopnum, neList[0].itemList_);
            getNEList_(locInputPairList, docIdList, totalCount, taxonomy_param_.max_locnum, neList[1].itemList_);
            getNEList_(orgInputPairList, docIdList, totalCount, taxonomy_param_.max_orgnum, neList[2].itemList_);

        }


    }
    catch ( std::exception& ex)
    {
        taxonomyRep.error_ = ex.what();
        return false;
    }
    return true;

}

void TaxonomyGenerationSubManager::getNEList_(std::vector<std::pair<labelid_t, docid_t> >& inputPairList
        , const std::vector<uint32_t>& docIdList, uint32_t totalDocCount, uint32_t max, std::vector<ne_item_type >& neItemList)
{
    std::sort(inputPairList.begin(), inputPairList.end() );
    uint32_t distinctCount = 0;
    uint32_t lastConceptId = 0;

    for ( std::size_t i=0;i<inputPairList.size();i++ )
    {
        if (inputPairList[i].first!=lastConceptId)
        {
            ++distinctCount;
            lastConceptId = inputPairList[i].first;
        }

    }
    //label str, partialDF, global DF
    typedef boost::tuple<izenelib::util::UString, boost::dynamic_bitset<>, uint32_t> NEInfo;
    std::vector<std::pair<double, NEInfo> > neList(distinctCount, std::make_pair(0.0, boost::make_tuple(izenelib::util::UString("",izenelib::util::UString::UTF_8),boost::dynamic_bitset<>(docIdList.size()),0) ) );
    if ( neList.size() == 0 ) return;
    lastConceptId = 0;
    uint32_t p=0;
    for ( std::size_t i=0;i<inputPairList.size();i++ )
    {
        if ( inputPairList[i].first != lastConceptId )
        {
            labelManager_->getLabelStringByLabelId(inputPairList[i].first, neList[p].second.get<0>() );

            uint32_t globalDF = 0;
            labelManager_->getLabelDF( inputPairList[i].first, globalDF );
            neList[p].second.get<2>() = globalDF;
            ++p;
        }

        neList[p-1].second.get<1>().set(inputPairList[i].second);
        lastConceptId = inputPairList[i].first;
    }
    for ( std::size_t i=0;i<neList.size();i++ )
    {
        neList[i].first = NERRanking::getScore( neList[i].second.get<0>(), neList[i].second.get<1>().count(),
                                                neList[i].second.get<2>(), totalDocCount);
//         std::cout<<"score : "<<neList[i].first<<std::endl;
    }
    std::sort(neList.begin(), neList.end(), std::greater<std::pair<double, NEInfo> >() );
    uint32_t size = neList.size();
    for (uint32_t i=neList.size(); i>0;i--)
    {
        if (neList[i-1].first > 0.0) break;
        size--;
    }
    uint32_t count = max>size?size:max;
    neItemList.resize(count);
    for (uint32_t i=0;i<count;i++)
    {
        neItemList[i].first = neList[i].second.get<0>();
        neItemList[i].second.resize( neList[i].second.get<1>().count() );
        uint32_t index = 0;
        for (uint32_t p=0;p<docIdList.size();p++)
        {
            if ( neList[i].second.get<1>()[p] )
            {
                neItemList[i].second[index] = docIdList[p];
                ++index;
            }
        }
    }
}

