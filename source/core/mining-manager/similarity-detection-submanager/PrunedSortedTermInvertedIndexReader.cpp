/**
 * @file core/mining-manager/similarity-detection-submanager/PrunedSortedTermInvertedIndexReader.cpp
 * @author Ian Yang&& Jinglei
 * @date Created <2009-10-12 12:09:26>
 * @date Updated <2009-11-04 16:41:33>
 */
#include "PrunedSortedTermInvertedIndexReader.h"

#include <util/functional.h>

#include <queue>
#include <limits>

namespace sf1r {
namespace sim {
PrunedSortedTermInvertedIndexReader::PrunedSortedTermInvertedIndexReader(
    float termNumLimit,
    float docNumLimit,
    IndexReader* reader,
    propertyid_t fieldId,
    const std::string& field,
    size_t numDocs,
    float avgDocLen
)
: indexReader_(reader)
, termReader_()
, fieldId_(fieldId)
, field_(field)
, idFreqList_()
, cursor_(0)
, numDocs_(numDocs)
, avgDocLen_(avgDocLen)
{

    float docLimits=0;
    if(numDocs<=1000)
        docLimits=docNumLimit;
    else if(numDocs<=10000)
        docLimits=300;
    else if(numDocs<=100000)
        docLimits=600;
    else
        docLimits=1000;

    static const collectionid_t kCollectionId = 1;
    if (indexReader_)
    {
        termReader_.reset(indexReader_->getTermReader(kCollectionId));
    }

    if (termReader_ )
    {
        typedef std::pair<id_type, size_type> value_type;
        typedef std::vector<value_type> sequence_type;
        typedef izenelib::util::second_less<value_type> less_than;
        std::priority_queue<value_type, sequence_type, less_than> queue;

        boost::scoped_ptr<TermIterator> termIterator(
            termReader_->termIterator(field_.c_str())
        );
        if (termIterator)
        {
            while (termIterator->next())
            {
            	if ((termIterator->termInfo()->docFreq() > 1)/*&&
            	        (termIterator->termInfo()->docFreq() <=docLimits)*/)
            	        /*((float)termIterator->termInfo()->docFreq()/(numDocs+1))<0.1)*/
                {
                    std::pair<id_type, size_type> element(
                        termIterator->term()->value,
                        termIterator->termInfo()->docFreq()
                    );

                    if (queue.size() < termNumLimit)
                    {
                        queue.push(element);
                    }
                    else if (element.second < queue.top().second)
                    {
                        queue.push(element);
                        queue.pop();
                    }
                }
            }
        }
        std::cout<<"Term number after pruning: "<<queue.size()<<std::endl;


        idFreqList_.reserve(queue.size());
        while (!queue.empty())
        {
            idFreqList_.push_back(queue.top());
            queue.pop();
        }
    }
}

void PrunedSortedTermInvertedIndexReader::getDocWeightList(
    doc_weight_list_type& result
)
{
    const uint32_t weightThreshold=6;
    float fieldWeight;
    if(field_=="tg_label")
        fieldWeight=3;
    else if(indexReader_->getAveragePropertyLength(fieldId_)<10)
        fieldWeight=1.4;
    else
        fieldWeight=1;
    BOOST_ASSERT(0 < cursor_ && cursor_ <= idFreqList_.size());
    doc_weight_list_type tmp;

    izenelib::ir::indexmanager::Term term(field_);
    term.setValue(idFreqList_[cursor_ - 1].first);
    if (indexReader_ && termReader_ && termReader_->seek(&term))
    {
        boost::scoped_ptr<izenelib::ir::indexmanager::TermDocFreqs>
            tfReader(termReader_->termDocFreqs());
        if (tfReader)
        {
            int termNum=0;
            int remainedNum=0;
            while (tfReader->next())
            {
                std::size_t docLen = indexReader_->docLength(tfReader->doc(), fieldId_);
                float weight = 0.0F;
                if (docLen > 0)
                {

                    float df=getDocFreq();
                    float dfNorm=log((numDocs_-df+0.5)/(df+0.5));
                    float tf=tfReader->freq();
                    float tfNormDenorm=default_k1_*((1-default_b_)+default_b_*docLen/avgDocLen_)+tf;
                    float tfNorm= (default_k1_+1)*tf/tfNormDenorm;
                    weight=tfNorm*dfNorm;
//                    std::cout<<"weight in DOC list: "<<weight<<std::endl;
                }

                if(weight>=weightThreshold)
                {
                    tmp.push_back(
                            std::make_pair(tfReader->doc(), weight*fieldWeight) );
                    remainedNum++;
                }
                termNum++;
            }
//            std::cout<<"The pruning ratio of docs in a term is : "<<(float)(termNum-remainedNum)/termNum<<std::endl;
        }
    }

    result.swap(tmp);
//    std::cout<<"size of doc weight list: "<<result.size()<<std::endl;
}

}} // namespace sf1r::sim
