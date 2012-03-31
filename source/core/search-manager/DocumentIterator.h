/**
 * @file sf1r/search-manager/DocumentIterator.h
 * @author Yingfeng Zhang
 * @date Created <2009-09-21>
 * @brief Interface for DocumentIterator
 */
#ifndef DOCUMENT_ITERATOR_H
#define DOCUMENT_ITERATOR_H

#include <common/type_defs.h>

#include <ranking-manager/RankingManager.h>
#include <ranking-manager/RankQueryProperty.h>
#include <ranking-manager/RankDocumentProperty.h>

#define MAX_DOC_ID      0xFFFFFFFF
#define MAX_COUNT       0xFFFFFFFF // max value of count_t type

#define PREFETCH_TERMID 1

#define SKIP_ENABLED 1

namespace sf1r {

class DocumentIterator
{
public:
    DocumentIterator():current_(false), not_(false), scorer_(false){}

    virtual ~DocumentIterator(){}

public:
    virtual void add(DocumentIterator* pDocIterator) = 0;

    virtual bool next() = 0;

    virtual docid_t doc() = 0;

    ///propIndex is only used for VirtualTermDocIterator, which is a cross property iterator
    virtual void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0) = 0;

    virtual void df_cmtf(DocumentFrequencyInProperties& dfmap,
                             CollectionTermFrequencyInProperties& ctfmap,
                             MaxTermFrequencyInProperties& maxtfmap) = 0;

    virtual count_t tf() = 0;

    ///if skip list is supported within index, this function would be a virtual one, too
    virtual docid_t skipTo(docid_t target)
    {
        docid_t currDoc;
        do
        {
            if(!next())
                return MAX_DOC_ID;
            currDoc = doc();
        } while(target > currDoc);

        return currDoc;
    }

    virtual void queryBoosting(double& score, double& weight) {}

    virtual void print(int level = 0) {}

    virtual double score(
        const RankQueryProperty& rankQueryProperty,
        const boost::shared_ptr<PropertyRanker>& propertyRanker
    ){ return 0.0f; }


    void setCurrent(bool current){ current_ = current; }

    bool isCurrent(){ return current_; }

    void setNot(bool isNot) { not_ = isNot; }

    bool isNot() { return not_; }

    bool isScorer() { return scorer_;}
protected:
    bool current_;

    bool not_; ///whether it is NOT iterator

    bool scorer_;
};

}

#endif
