/**
 * @file sf1r/search-manager/ORDocumentIterator.h
 * @author Yingfeng Zhang
 * @date Created <2009-09-22>
 * @date Updated <2010-03-24 14:42:03>
 * @brief DocumentIterator which implements the OR semantics
 * NOT semantics is also included
 */
#ifndef OR_DOCUMENT_ITERATOR_H
#define OR_DOCUMENT_ITERATOR_H

#include "DocumentIterator.h"

#include <ir/index_manager/utility/PriorityQueue.h>

#include <vector>

namespace sf1r{
class NOTDocumentIterator;
class ORDocumentIterator:public DocumentIterator
{
public:
    class DocumentIteratorQueue : public izenelib::ir::indexmanager::PriorityQueue<DocumentIterator*>
    {
    public:
        DocumentIteratorQueue(size_t size)
        {
            initialize(size,false);
        }
    protected:
        bool lessThan(DocumentIterator* o1, DocumentIterator* o2)
        {
            return o1->doc() < o2->doc();
        }
    };

public:
    ORDocumentIterator();

    virtual ~ORDocumentIterator();

public:
    void add(DocumentIterator* pDocIterator);

    void add(VirtualPropertyTermDocumentIterator* pDocIterator);

    bool next();

    docid_t doc(){ return currDoc_; }

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0);

    void df_cmtf(
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap);

    count_t tf();

    bool empty() { return docIteratorList_.empty(); }

    void queryBoosting(double& score, double& weight);

    void print(int level=0);

#if SKIP_ENABLED
    docid_t skipTo(docid_t target);

protected:
    docid_t do_skipTo(docid_t target);
#endif
protected:
    virtual void initDocIteratorQueue();

    bool do_next();


private:
    inline bool move_together_with_not();

protected:
    std::vector<DocumentIterator*> docIteratorList_;

    DocumentIteratorQueue* pDocIteratorQueue_;

    docid_t currDoc_;

private:

    bool hasNot_;

    docid_t currDocOfNOTIter_;

    bool initNOTIterator_;

    NOTDocumentIterator* pNOTDocIterator_;
};

}

#endif
