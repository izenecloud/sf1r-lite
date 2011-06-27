#include "PrunedSortedForwardIndex.h"

#include <ir/index_manager/index/TermDocFreqs.h>
#include <util/PriorityQueue.h>

#include <limits>

namespace sf1r{

#define MAX_MEMORY_USED 1024*1024*100

using izenelib::ir::indexmanager::TermDocFreqs;

class WeightQueue : public izenelib::util::PriorityQueue<std::pair<uint32_t,float> >
{
public:
    WeightQueue(size_t size)
    {
        this->initialize(size);
    }
protected:
    bool lessThan(std::pair<uint32_t,float> o1, std::pair<uint32_t,float> o2)
    {
        return (o1.second < o2.second);
    }
};

PrunedSortedForwardIndex::PrunedSortedForwardIndex(
    const std::string& workingDir,
    IndexReader* reader,
    unsigned int prunedBound,
    unsigned int docNum,
    unsigned int maxDoc
)
    :workingDir_(workingDir)
    ,indexReader_(reader)    
    ,prunedBound_(prunedBound)
    ,forwardIndexPath_(boost::filesystem::path(boost::filesystem::path(workingDir)/"forward").string())
    ,forwardIndex_(docNum*1000*8 > MAX_MEMORY_USED ? MAX_MEMORY_USED : docNum*1000*8, 
                           forwardIndexPath_)
    ,prunedForwardContainer_(workingDir)
    ,numDocs_(docNum)
    ,maxDoc_(maxDoc)
{
    static const uint32_t kCollectionId = 1;
    if (indexReader_)
    {
        termReader_.reset(indexReader_->getTermReader(kCollectionId));
    }
    prunedForwardContainer_.open();
}

PrunedSortedForwardIndex::~PrunedSortedForwardIndex()
{

}

void PrunedSortedForwardIndex::build()
{
    buildForward_();
    buildPrunedForward_();
}

void PrunedSortedForwardIndex::buildForward_()
{
    if(fields_.empty()) return;
    std::map<std::pair<std::string, uint32_t>, std::pair<float,float> >::iterator fit = fields_.begin();
    for(; fit != fields_.end(); ++fit)
    {
        std::string field = fit->first.first;
        uint32_t fieldId = fit->first.second;
        float fieldWeight = fit->second.first;
        float avgDocLen = fit->second.second;
        boost::scoped_ptr<TermIterator> termIterator(termReader_->termIterator(field.c_str()));
        boost::scoped_ptr<TermDocFreqs> termDocFreq(new TermDocFreqs);
        if(termIterator)
        {
            while(termIterator->next())
            {
                float df = termIterator->termInfo()->docFreq();
                if (df > 1)
                {
                    termDocFreq->reset(termIterator->termPosting(),*(termIterator->termInfo()),false);
                    while(termDocFreq->next())
                    {
                    	   std::size_t docLen = indexReader_->docLength(termDocFreq->doc(), fieldId);
                    	   float weight = 0.0F;
                    	   if (docLen > 0)
                    	   {
                    	       float dfNorm=std::log((numDocs_-df+0.5)/(df+0.5));
                    	       float tf=termDocFreq->freq();
                    	       float tfNormDenorm=default_k1_*((1-default_b_)+default_b_*docLen/avgDocLen)+tf;
                    	       float tfNorm= (default_k1_+1)*tf/tfNormDenorm;
                    	       weight=tfNorm*dfNorm;
                    	   }
                         weight = termDocFreq->freq()*std::log(numDocs_/df);
                         forwardIndex_.incre(termDocFreq->doc(), termIterator->term()->value, weight*fieldWeight);
                    }
                }
            }
        }
    }
}

void PrunedSortedForwardIndex::buildPrunedForward_()
{
    forwardIndex_.clear();//save memory
    typedef izenelib::am::MatrixDB<uint32_t, float>::row_type RowType;
	
    for(uint32_t did = 1; did <= maxDoc_; ++did)
    {
        RowType row_data;
        if(forwardIndex_.row_without_cache(did,row_data))
        {
            WeightQueue queue(prunedBound_);
            RowType::iterator rit = row_data.begin();
            for(; rit != row_data.end(); ++rit)
            {
                queue.insert(std::make_pair(rit->first,rit->second));
            }
            PrunedForwardType prunedForward;
            prunedForward.reserve(prunedBound_);
            size_t size = queue.size() < prunedBound_ ? queue.size() : prunedBound_;
            float sum = 0;
            for(size_t i = 0; i < size; ++i)
            {
                std::pair<uint32_t, float> element = queue.pop();
                sum += element.second * element.second;
                prunedForward.push_back(element);
            }
            if(sum > 0) 
            {
                sum = std::sqrt(sum);
                for(PrunedForwardType::iterator fit = prunedForward.begin(); fit != prunedForward.end(); ++fit)
                {
                    fit->second /= sum;
                }
/*				
                std::cout<<"prun doc "<<did<<" ";
                for(PrunedForwardType::iterator fit = prunedForward.begin(); fit != prunedForward.end(); ++fit)
                {
                    fit->second /= sum;
                    std::cout<<"<"<<fit->first<<","<<fit->second<<"> ";
                }
                std::cout<<std::endl;
*/
                prunedForwardContainer_.insert(did, prunedForward);
            }
        }
    }
    //boost::filesystem::path path(forwardIndexPath_);
    //boost::filesystem::remove_all(path);
}

}
