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
#include <net/aggregator/Util.h>
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
        , algorithm_(), tgParams_()
{
    tgParams_.topDocNum_ = taxonomy_param_.top_docnum;
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

bool TaxonomyGenerationSubManager::GetConceptsByDocidList(const std::vector<uint32_t>& rawDocIdList, const izenelib::util::UString& queryStr,
uint32_t totalDocCount, idmlib::cc::CCInput32& input)
{
    std::vector<uint32_t> docIdList(rawDocIdList);
    if(docIdList.size()> tgParams_.topDocNum_ )
    {
        docIdList.resize( tgParams_.topDocNum_);
    }
    if ( docIdList.size()==0 ) return true;
    uint32_t docCount = docIdList.size();
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
    uint32_t p=0;
    for ( uint32_t i=0;i<all_label_list.size();i++)
    {
        for (uint32_t j=0;j<all_label_list[i].size();j++)
        {
            uint32_t labelId = all_label_list[i][j];
            inputPairList[p].first = labelId;
            inputPairList[p].second = i;
            ++p;
            
        }
    }
    std::sort(inputPairList.begin(), inputPairList.end() );
    uint32_t distinctCount = 0;
    uint32_t lastConceptId = 0;

    for ( std::size_t i=0;i<inputPairList.size();i++ )
    {
        if(inputPairList[i].first!=lastConceptId)
        {
            ++distinctCount;
            lastConceptId = inputPairList[i].first;
        }

    }
    
    std::vector<ScoreItem> score_items(distinctCount, ScoreItem(docIdList.size()));
    
    lastConceptId = 0;
    p=0;
    for ( std::size_t i=0;i<inputPairList.size();i++ )
    {
        if( inputPairList[i].first != lastConceptId )
        {
            score_items[p].concept_id = inputPairList[i].first;
            ++p;
            
        }

        score_items[p-1].doc_invert.set(inputPairList[i].second);
        lastConceptId = inputPairList[i].first;
    }
    if( showtime )
    {
        std::cout<<"[TG phase 3-1-3]: "<<clocker.elapsed()<<" seconds."<<std::endl;
        clocker.restart();
    }
    if( showtime )
    {
        std::cout<<"[TG phase 3-1]: "<<clocker.elapsed()<<" seconds."<<std::endl;
        clocker.restart();
        std::cout<<"TG: original "<<score_items.size() <<" concepts."<<std::endl;
    }
    

    for ( std::size_t i=0;i<score_items.size();i++ )
    {

        uint32_t partialDF = score_items[i].doc_invert.count();
        if ( partialDF < tgParams_.minDocSupp_ )
        {
            continue;
        }
        //compute score
        
        uint32_t globalDF =1;
        float inc = totalDocCount;
        inc = inc/10000;
        if(inc<2) inc = 2;
        labelManager_->getConceptDFByConceptId(score_items[i].concept_id, globalDF);
        score_items[i].score =log(partialDF+1)/log((sqrt(globalDF+1)+1)/log(totalDocCount+2)+2);


        score_items[i].score2rank = log(partialDF+1)/log((globalDF+inc)/log(sqrt(totalDocCount+1)+1)+1);

        //Because the top K value is hard coded in SIA now,
        // so just also hard coded in MIA.
        //This should be made configurable further.
        float thresh=(float)docCount/3.5;
        if(docCount>10&&partialDF>thresh)
        {
            score_items[i].score =0;
            score_items[i].score2rank=0;
        }

    }
    if( showtime )
    {
        std::cout<<"[TG phase 3-2]: "<<clocker.elapsed()<<" seconds."<<std::endl;
        clocker.restart();
    }
    std::sort(score_items.begin(),score_items.end() );
    uint32_t concept_count = score_items.size()>tgParams_.canConceptNum_?tgParams_.canConceptNum_:score_items.size();

    //GetConceptsByDocidList is called locally, so worker_id set to 0
    input.doc_list.resize(docIdList.size());
    for(uint32_t i=0;i<input.doc_list.size();i++)
    {
        input.doc_list[i] = net::aggregator::Util::GetWDocId(0, docIdList[i]);
    }
    input.query = queryStr;
    analyzer_->GetTermList(queryStr, input.query_term_list, input.query_termid_list);
    std::vector<idmlib::cc::ConceptItem>& concept_list = input.concept_list;
    concept_list.resize(concept_count, idmlib::cc::ConceptItem(docIdList.size()) );
    for(uint32_t i=0;i<concept_count;i++)
    {
        concept_list[i].doc_invert = score_items[i].doc_invert;
        labelManager_->getConceptStringByConceptId( score_items[i].concept_id, concept_list[i].text );
        labelManager_->getLabelType(score_items[i].concept_id, concept_list[i].type);
        
        if( concept_list[i].text.length()>0 && concept_list[i].text.isKoreanChar(0) && concept_list[i].text.length()<4)
        {
             concept_list[i].score *= 0.33;
        }
        analyzer_->GetTermList(concept_list[i].text, concept_list[i].term_list, concept_list[i].termid_list);
        concept_list[i].score = score_items[i].score;
        
    }
    return true;
}

bool TaxonomyGenerationSubManager::GetResult(const idmlib::cc::CCInput64& input, TaxonomyRep& taxonomyRep ,NEResultList& neList)
{
    algorithm_.DoClustering(input, tgParams_, taxonomyRep.result_);
    //TODO NE result
    neList.resize(3);
    neList[0].type = izenelib::util::UString("PEOPLE", izenelib::util::UString::UTF_8);
    neList[1].type = izenelib::util::UString("LOCATION", izenelib::util::UString::UTF_8);
    neList[2].type = izenelib::util::UString("ORGANIZATION", izenelib::util::UString::UTF_8);
    if ( taxonomy_param_.enable_nec )
    {
        idmlib::cc::CCInput64 peop_input;
        idmlib::cc::CCInput64 loc_input;
        idmlib::cc::CCInput64 org_input;
        for(uint32_t i=0;i<input.concept_list.size();i++)
        {
            if(input.concept_list[i].type == LabelManager::LABEL_TYPE::PEOP )
            {
                peop_input.concept_list.push_back(input.concept_list[i]);
            }
            else if(input.concept_list[i].type == LabelManager::LABEL_TYPE::LOC )
            {
                loc_input.concept_list.push_back(input.concept_list[i]);
            }
            else if(input.concept_list[i].type == LabelManager::LABEL_TYPE::ORG )
            {
                org_input.concept_list.push_back(input.concept_list[i]);
            }
        }
        
        if(!peop_input.concept_list.empty())
        {
            GetNEResult_(peop_input, input.doc_list, neList[0].item_list);
        }
        
        if(!loc_input.concept_list.empty())
        {
            GetNEResult_(loc_input, input.doc_list, neList[1].item_list);
        }
        
        if(!org_input.concept_list.empty())
        {
            GetNEResult_(org_input, input.doc_list, neList[2].item_list);
        }
    }
    return true;
}


void TaxonomyGenerationSubManager::AggregateInput(const std::vector<std::pair<uint32_t, idmlib::cc::CCInput32> >& input_list, idmlib::cc::CCInput64& result)
{
    if(input_list.empty()) return;
    //set some basic info
    result.query = input_list[0].second.query;
    result.query_term_list = input_list[0].second.query_term_list;
    result.query_termid_list = input_list[0].second.query_termid_list;
    
    uint32_t all_docid_count = 0;
    for(uint32_t i=0;i<input_list.size();i++)
    {
        all_docid_count += input_list[i].second.doc_list.size();
    }
    
    std::vector<idmlib::cc::ConceptItem>& r_concept_list = result.concept_list;
    std::vector<uint64_t>& r_doc_list = result.doc_list;
    izenelib::am::rde_hash<izenelib::util::UString, uint32_t> app_concept;
//     std::vector<std::pair<double, uint32_t> > score_list;
    
//     uint32_t p = 0;
    for(uint32_t i=0;i<input_list.size();i++)
    {
        uint32_t worker_id = input_list[i].first;
        const idmlib::cc::CCInput32& input = input_list[i].second;
        const std::vector<uint32_t>& doc_list = input.doc_list;
        //expand doc_list
        uint32_t current_doc_size = r_doc_list.size();
        r_doc_list.resize(current_doc_size+doc_list.size());
        for(uint32_t j=0;j<doc_list.size();j++)
        {
            r_doc_list[current_doc_size+j] = net::aggregator::Util::GetWDocId(worker_id, doc_list[j]);
        }
        
        const std::vector<idmlib::cc::ConceptItem>& concept_list = input.concept_list;
        for(uint32_t j=0;j<concept_list.size();j++)
        {
            idmlib::cc::ConceptItem new_concept_item(concept_list[j]);
            new_concept_item.doc_invert.resize(all_docid_count);
            new_concept_item.doc_invert = new_concept_item.doc_invert<<=current_doc_size;
            uint32_t* same_one = app_concept.find(concept_list[j].text);
            if(same_one!=NULL)
            {
                CombineConceptItem_(new_concept_item, r_concept_list[*same_one] );
            }
            else
            {
                r_concept_list.push_back(new_concept_item);
                app_concept.insert( concept_list[j].text, r_concept_list.size()-1 );
            }

        }
    }
    
    
    
}

void TaxonomyGenerationSubManager::CombineConceptItem_(const idmlib::cc::ConceptItem& from, idmlib::cc::ConceptItem& to)
{
    to.doc_invert ^= from.doc_invert;
    if(from.score > to.score) to.score = from.score;
    if(from.min_query_distance < to.min_query_distance ) to.min_query_distance = from.min_query_distance;
}

void TaxonomyGenerationSubManager::GetNEResult_(const idmlib::cc::CCInput64& input, const std::vector<wdocid_t>& doc_list, std::vector<NEItem>& item_list)
{
    item_list.resize(input.concept_list.size());
    for(uint32_t i=0;i<item_list.size();i++)
    {
        item_list[i].text = input.concept_list[i].text;
        item_list[i].doc_list.resize(input.concept_list[i].doc_invert.count());
        uint32_t p = 0;
        for(uint32_t j=0; j<doc_list.size(); j++)
        {
            if( input.concept_list[i].doc_invert[j] )
            {
                item_list[i].doc_list[p] = doc_list[j];
                ++p;
            }
        }
    }
}


// bool TaxonomyGenerationSubManager::getQuerySpecificTaxonomyInfo (
//     const std::vector<docid_t>&  docIdList,
//     const izenelib::util::UString& queryStr,
//     uint32_t totalCount,
//     uint32_t numberFromSia,
//     TaxonomyRep& taxonomyRep
//     ,ne_result_list_type& neList)
// {
//     try
//     {
//         if ( docIdList.size()==0 ) return true;
//         bool showtime = true;
//         izenelib::util::ClockTimer clocker;
//         std::vector<std::vector<labelid_t> > all_label_list;
//         labelManager_->getLabelsByDocIdList( docIdList, all_label_list);
//         if (showtime)
//         {
//             std::cout<<"[TG phase 2]: "<<clocker.elapsed()<<" seconds."<<std::endl;
//             clocker.restart();
//         }
//         if ( all_label_list.size() == 0 )
//         {
//             taxonomyRep.error_ = "Can not get any labels, label list is empty.";
//             return false;
//         }
// 
//         uint32_t tsiz = 0;
//         for ( uint32_t i=0;i<all_label_list.size();i++)
//         {
//             tsiz += all_label_list[i].size();
//         }
//         if ( tsiz==0 )
//         {
//             //no labels to clustering
//             return true;
//         }
//         std::vector<std::pair<labelid_t, docid_t> > inputPairList(tsiz);
//         std::vector<std::pair<labelid_t, docid_t> > peopInputPairList;
//         std::vector<std::pair<labelid_t, docid_t> > locInputPairList;
//         std::vector<std::pair<labelid_t, docid_t> > orgInputPairList;
//         uint32_t p=0;
//         for ( uint32_t i=0;i<all_label_list.size();i++)
//         {
//             for (uint32_t j=0;j<all_label_list[i].size();j++)
//             {
//                 uint32_t labelId = all_label_list[i][j];
//                 inputPairList[p].first = labelId;
//                 inputPairList[p].second = i;
//                 ++p;
//                 uint8_t type;
//                 if ( labelManager_->getLabelType(labelId, type) )
//                 {
//                     if ( type == LabelManager::LABEL_TYPE::PEOP )
//                     {
//                         peopInputPairList.push_back( std::make_pair(labelId, i) );
//                     }
//                     else if ( type == LabelManager::LABEL_TYPE::LOC )
//                     {
//                         locInputPairList.push_back( std::make_pair(labelId, i) );
//                     }
//                     else if ( type == LabelManager::LABEL_TYPE::ORG )
//                     {
//                         orgInputPairList.push_back( std::make_pair(labelId, i) );
//                     }
//                 }
//             }
//         }
//         
//         idmlib::cc::ConceptClusteringInput input;
//         GetClusteringInput_(inputPairList, queryStr, docIdList, totalCount, input);
//         
//         algorithm_.DoClustering(input, tgParams_, taxonomyRep.result_ );
// //         std::vector<boost::shared_ptr<TgClusterRep> > nerResult;
// 
//         neList.resize(3);
//         neList[0].type_ = izenelib::util::UString("PEOPLE", izenelib::util::UString::UTF_8);
//         neList[1].type_ = izenelib::util::UString("LOCATION", izenelib::util::UString::UTF_8);
//         neList[2].type_ = izenelib::util::UString("ORGANIZATION", izenelib::util::UString::UTF_8);
// 
// //         {
// //             algorithm_.doClustering(peopInputPairList, docIdList, totalCount, peopParams_, nerResult );
// //             for(uint32_t i=0;i<nerResult.size();i++)
// //             {
// //                 neList[0].second.push_back( nerResult[i]->name_ );
// //             }
// //             nerResult.clear();
// //         }
// //
// //         {
// //             algorithm_.doClustering(locInputPairList, docIdList, totalCount, locParams_, nerResult );
// //             for(uint32_t i=0;i<nerResult.size();i++)
// //             {
// //                 neList[1].second.push_back( nerResult[i]->name_ );
// //             }
// //             nerResult.clear();
// //         }
// //
// //         {
// //             algorithm_.doClustering(orgInputPairList, docIdList, totalCount, orgParams_, nerResult );
// //             for(uint32_t i=0;i<nerResult.size();i++)
// //             {
// //                 neList[2].second.push_back( nerResult[i]->name_ );
// //             }
// //             nerResult.clear();
// //         }
// 
//         if ( taxonomy_param_.enable_nec )
//         {
//             getNEList_(peopInputPairList, docIdList, totalCount, taxonomy_param_.max_peopnum, neList[0].itemList_);
//             getNEList_(locInputPairList, docIdList, totalCount, taxonomy_param_.max_locnum, neList[1].itemList_);
//             getNEList_(orgInputPairList, docIdList, totalCount, taxonomy_param_.max_orgnum, neList[2].itemList_);
// 
//         }
// 
// 
//     }
//     catch ( std::exception& ex)
//     {
//         taxonomyRep.error_ = ex.what();
//         return false;
//     }
//     return true;
// 
// }


// void TaxonomyGenerationSubManager::getNEList_(std::vector<std::pair<labelid_t, docid_t> >& inputPairList
//         , const std::vector<uint32_t>& docIdList, uint32_t totalDocCount, uint32_t max, std::vector<ne_item_type >& neItemList)
// {
//     std::sort(inputPairList.begin(), inputPairList.end() );
//     uint32_t distinctCount = 0;
//     uint32_t lastConceptId = 0;
// 
//     for ( std::size_t i=0;i<inputPairList.size();i++ )
//     {
//         if (inputPairList[i].first!=lastConceptId)
//         {
//             ++distinctCount;
//             lastConceptId = inputPairList[i].first;
//         }
// 
//     }
//     //label str, partialDF, global DF
//     typedef boost::tuple<izenelib::util::UString, boost::dynamic_bitset<>, uint32_t> NEInfo;
//     std::vector<std::pair<double, NEInfo> > neList(distinctCount, std::make_pair(0.0, boost::make_tuple(izenelib::util::UString("",izenelib::util::UString::UTF_8),boost::dynamic_bitset<>(docIdList.size()),0) ) );
//     if ( neList.size() == 0 ) return;
//     lastConceptId = 0;
//     uint32_t p=0;
//     for ( std::size_t i=0;i<inputPairList.size();i++ )
//     {
//         if ( inputPairList[i].first != lastConceptId )
//         {
//             labelManager_->getLabelStringByLabelId(inputPairList[i].first, neList[p].second.get<0>() );
// 
//             uint32_t globalDF = 0;
//             labelManager_->getLabelDF( inputPairList[i].first, globalDF );
//             neList[p].second.get<2>() = globalDF;
//             ++p;
//         }
// 
//         neList[p-1].second.get<1>().set(inputPairList[i].second);
//         lastConceptId = inputPairList[i].first;
//     }
//     for ( std::size_t i=0;i<neList.size();i++ )
//     {
//         neList[i].first = NERRanking::getScore( neList[i].second.get<0>(), neList[i].second.get<1>().count(),
//                                                 neList[i].second.get<2>(), totalDocCount);
// //         std::cout<<"score : "<<neList[i].first<<std::endl;
//     }
//     std::sort(neList.begin(), neList.end(), std::greater<std::pair<double, NEInfo> >() );
//     uint32_t size = neList.size();
//     for (uint32_t i=neList.size(); i>0;i--)
//     {
//         if (neList[i-1].first > 0.0) break;
//         size--;
//     }
//     uint32_t count = max>size?size:max;
//     neItemList.resize(count);
//     for (uint32_t i=0;i<count;i++)
//     {
//         neItemList[i].first = neList[i].second.get<0>();
//         neItemList[i].second.resize( neList[i].second.get<1>().count() );
//         uint32_t index = 0;
//         for (uint32_t p=0;p<docIdList.size();p++)
//         {
//             if ( neList[i].second.get<1>()[p] )
//             {
//                 neItemList[i].second[index] = docIdList[p];
//                 ++index;
//             }
//         }
//     }
// }

