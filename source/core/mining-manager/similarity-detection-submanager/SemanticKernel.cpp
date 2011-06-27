#include "SemanticKernel.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <search-manager/SearchManager.h>
#include <la-manager/LAManager.h>
#include <query-manager/ActionItem.h>

#include <idmlib/similarity/all-pairs-similarity-search/data_set_iterator.h>
#include <idmlib/similarity/all-pairs-similarity-search/all_pairs_search.h>
#include <idmlib/similarity/all-pairs-similarity-search/all_pairs_output.h>
#include <idmlib/semantic_space/esa/SparseVectorSetFile.h>

namespace sf1r{

class SemanticKernelOutput : public idmlib::sim::AllPairsOutput
{
public:
    SemanticKernelOutput(
        boost::shared_ptr<DocumentManager> documentManager,
        const std::string& semanticField
        )
        :document_manager_(documentManager)
        ,semanticField_(semanticField)
        ,output_("result.txt", std::ios::out)
    {
    }

    ~SemanticKernelOutput() {}

public:
    void putPair(uint32_t id1, uint32_t id2, float weight)
    {
        std::cout<<"id1 "<<id1<<" id2 "<<id2<<" weight "<<weight<<std::endl;
        output_<<"id1 "<<id1<<" id2 "<<id2<<" weight "<<weight<<std::endl;		

        izenelib::util::UString rawText;
        if(document_manager_->getPropertyValue(id1, semanticField_, rawText))
        {
            rawText.displayStringValue(izenelib::util::UString::UTF_8);
            std::cout<<std::endl;
            std::string str;
            rawText.convertString(str, izenelib::util::UString::UTF_8);
            output_<<str<<std::endl;
        }
        izenelib::util::UString rawText2;
        if(document_manager_->getPropertyValue(id2, semanticField_, rawText2))
        {
            rawText2.displayStringValue(izenelib::util::UString::UTF_8);
            std::cout<<std::endl;			
            std::string str;
            rawText2.convertString(str, izenelib::util::UString::UTF_8);
            output_<<str<<std::endl;
        }
    }

    void finish()
    {}

private:
    boost::shared_ptr<DocumentManager> document_manager_;
    std::string semanticField_;
    std::ofstream output_;
};

SemanticKernel::SemanticKernel(
        const std::string& workingDir,
        boost::shared_ptr<DocumentManager> document_manager,
        boost::shared_ptr<IndexManager> index_manager,
        boost::shared_ptr<LAManager> laManager,
        boost::shared_ptr<IDManager> idManager,
        boost::shared_ptr<SearchManager> searchManager
    )
    :document_manager_(document_manager)
    ,index_manager_(index_manager)
    ,laManager_(laManager)
    ,idManager_(idManager)
    ,searchManager_(searchManager)
    ,allpairPath_(boost::filesystem::path(boost::filesystem::path(workingDir)/"allpair").string())
{
    analysisInfo_.analyzerId_ = "la_sia";
    analysisInfo_.tokenizerNameList_.insert("tok_divide");
    analysisInfo_.tokenizerNameList_.insert("tok_unite");
    boost::filesystem::create_directories(boost::filesystem::path(workingDir));	
    boost::filesystem::create_directories(boost::filesystem::path(allpairPath_));
    forwardIndexBuilder_ = new PrunedSortedForwardIndex(
        workingDir,
        index_manager_->getIndexReader(),
        50,
        index_manager_->getIndexReader()->numDocs(),
        document_manager_->getMaxDocId()
    );
}

SemanticKernel::~SemanticKernel()
{
    delete forwardIndexBuilder_;
}

void SemanticKernel::setSchema(
	const std::vector<uint32_t>& property_ids, 
	const std::vector<std::string>& properties, 
	const std::string& semanticField,
	const std::string& domainField
)
{
    semanticField_ = semanticField;
    domainField_ = domainField;
    size_t size = property_ids.size();
    for(size_t i = 0; i < size; ++i)
    {
        float fieldAveLength=index_manager_->getIndexReader()->getAveragePropertyLength(property_ids[i]);
        float weight = 0.3;
        if(properties[i] == semanticField_) weight = 0.6;
        forwardIndexBuilder_->addField(std::make_pair(properties[i], property_ids[i] ), weight, fieldAveLength);
        searchPropertyList_.push_back(properties[i]);
    }
    forwardIndexBuilder_->build();
}

bool SemanticKernel::buildQuery_(SearchKeywordOperation&action, izenelib::util::UString& queryUStr)
{
    action.clear();
    // Build raw Query Tree
    if ( !action.queryParser_.parseQuery( queryUStr, action.rawQueryTree_, action.unigramFlag_ ) )
        return false;
    //action.rawQueryTree_->print();

    std::vector<std::string>::const_iterator propertyIter = action.actionItem_.searchPropertyList_.begin();
    for (; propertyIter != action.actionItem_.searchPropertyList_.end(); propertyIter++)
    {
        // If there's already processed query, skip processing of this property..
        if ( action.queryTreeMap_.find( *propertyIter ) != action.queryTreeMap_.end()
                && action.propertyTermInfo_.find( *propertyIter ) != action.propertyTermInfo_.end() )
            continue;

        QueryTreePtr tmpQueryTree;
        if ( !action.queryParser_.getAnalyzedQueryTree(false, analysisInfo_, queryUStr, tmpQueryTree, true))
            return false;

        //tmpQueryTree->print();
        action.queryTreeMap_.insert( std::make_pair(*propertyIter,tmpQueryTree) );
        PropertyTermInfo ptInfo;
        tmpQueryTree->getPropertyTermInfo(ptInfo);
        action.propertyTermInfo_.insert( std::make_pair(*propertyIter,ptInfo) );
    }
    return true;
}

void SemanticKernel::doSearch()
{
    uint32_t maxDoc =document_manager_->getMaxDocId();
    Document document;
    KeywordSearchActionItem actionItem;
    actionItem.env_.encodingType_ = "UTF-8";
    actionItem.searchPropertyList_ = searchPropertyList_;
    actionItem.rankingType_ = sf1r::RankingType::BM25;
    std::vector<std::pair<std::string , bool> > sortPriorityList;
    static const std::pair<std::string , bool> kDefaultOrder("_rank", false);
    sortPriorityList.push_back(kDefaultOrder);
    actionItem.sortPriorityList_ = sortPriorityList;
    SearchKeywordOperation actionOperation(actionItem, true, laManager_, idManager_);
    PrunedForwardContainer& prunedForwardIndices = forwardIndexBuilder_->prunedForwardContainer_;

    string datafile = allpairPath_+"/allpair.vec";
    idmlib::ssp::SparseVectorSetOFileType simoutput(datafile);
    simoutput.open();
    for(uint32_t did = 1; did < maxDoc; ++did)
    {
	 try{
	 izenelib::util::UString queryUStr;
	 if(document_manager_->getPropertyValue(did, semanticField_, queryUStr))
	 {
            queryUStr.convertString(actionOperation.actionItem_.env_.queryString_, izenelib::util::UString::UTF_8);
            if(buildQuery_(actionOperation,  queryUStr))
            {
                std::vector<uint32_t> topKDocs;
                std::vector<float> topKRankScoreList;
                std::vector<float> topKCustomRankScoreList;
                std::size_t totalCount;

                if(searchManager_->search(
                    actionOperation,
                    topKDocs,
                    topKRankScoreList,
                    topKCustomRankScoreList,
                    totalCount,
                    100,
                    0
                 ))
                 //topKDocs.push_back(did);
                {
                    std::vector<uint32_t>::iterator dit = topKDocs.begin();
                    std::map<uint32_t, float> results;

                    for(; dit != topKDocs.end(); ++dit)
                    {
                        PrunedForwardType forward;
                        if(prunedForwardIndices.get(*dit, forward))
                        {
                            PrunedForwardType::iterator fit = forward.begin();
                            for(; fit != forward.end(); ++fit)
                            {
                                std::map<uint32_t, float>::iterator rit = results.find(fit->first);
                               
                                if(rit == results.end())
                                {
                                    results.insert(std::make_pair(fit->first,fit->second));
                                }
                                else
                                {
                                    rit->second += fit->second;
                                }
                            }
                        }
                    }
                    float sum = 0;
                   
                    idmlib::ssp::SparseVectorType normalized(did, results.size());
                    for(std::map<uint32_t, float>::iterator rit = results.begin(); rit != results.end(); ++rit)
                    {
                        float w = rit->second /topKDocs.size();
                        sum += w*w;
                        normalized.insertItem(rit->first, w);
                    }
                    sum = std::sqrt(sum);
                    if(sum > 0) 
                    {
                        sum = std::sqrt(sum);
                        for(idmlib::ssp::SparseVectorType::list_iter_t fit = normalized.list.begin(); fit != normalized.list.end(); ++fit)
                        {
                            fit->value /= sum;
                        }
                        simoutput.put(normalized);
                    }
                }
            }
        }
        }catch (std::exception& e)
        	{}
    }
    simoutput.close();
    boost::shared_ptr<idmlib::sim::DataSetIterator> dataSetIterator(new idmlib::sim::SparseVectorSetIterator(datafile));
    boost::shared_ptr<idmlib::sim::DocSimOutput> output(new idmlib::sim::DocSimOutput(allpairPath_+"/result"));
    idmlib::sim::AllPairsSearch allPairs(output, 0.8);
    allPairs.findAllSimilarPairs(dataSetIterator, 0);
    output->finish();

    std::fstream out("result.txt", std::ios::out);

    std::vector<std::pair<uint32_t, float> > result;
    for (size_t idx =1 ; idx <= maxDoc; idx++) 
    {
        output->getSimilarDocIdScoreList(idx,10,result);
        if(! result.empty())
        {
            izenelib::util::UString domain;
            document_manager_->getPropertyValue(idx, domainField_, domain);
            bool has = false;
            for(std::vector<std::pair<uint32_t,float> >::iterator it = result.begin(); it != result.end(); ++it)
            {
                izenelib::util::UString thisDomain;
                document_manager_->getPropertyValue(it->first, domainField_, thisDomain);
                if(thisDomain == domain) continue;
                has = true;
                flushSim_(it->first, it->second, out);
            }
            if(has)
            {
                flushSim_(idx, 0, out);
                out<<"\n\r";
            }
        }
    }
}

void SemanticKernel::flushSim_(uint32_t did, float weight, std::fstream& out)
{
    izenelib::util::UString rawText;
    izenelib::util::UString domain;
    document_manager_->getPropertyValue(did, domainField_, domain);
    std::string domainstr;
    domain.convertString(domainstr, izenelib::util::UString::UTF_8);	
    if(document_manager_->getPropertyValue(did, semanticField_, rawText))
    {
        std::string str;
        rawText.convertString(str, izenelib::util::UString::UTF_8);
        if(weight) out<<did<<"	"<<weight<<"	"<<str<<"	source	"<<domainstr<<std::endl;
        else out<<did<<"	"<<str<<"	source	"<<domainstr<<std::endl;
    }
}

}

