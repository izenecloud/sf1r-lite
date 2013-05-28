/**
 * @file DocumentIteratorContainer.h
 * @brief a container of DocumentIterator instances.
 */

#ifndef SF1R_DOCUMENT_ITERATOR_CONTAINER_H
#define SF1R_DOCUMENT_ITERATOR_CONTAINER_H

#include <vector>

namespace sf1r
{
class DocumentIterator;

class DocumentIteratorContainer
{
public:
    DocumentIteratorContainer()
        :hasNullDocIterator_(false){}
    ~DocumentIteratorContainer();

    /**
     * Add @p docIter into the container.
     */
    void add(DocumentIterator* docIter);

    /**
     * Combine the added instances into one DocumentIterator.
     *
     * If there is only one instance added, it would return it directly,
     * avoiding the cost of @c CombinedDocumentIterator for multiple instances.
     *
     * @return the combined DocumentIterator.
     * @post the container would be empty after this function is called.
     */
    DocumentIterator* combine();

private:
    std::vector<DocumentIterator*> docIters_;
    bool hasNullDocIterator_;
};

} // namespace sf1r

#endif // SF1R_DOCUMENT_ITERATOR_CONTAINER_H
