#include "DupDetector.h"
#include <util/izene_log.h>
#include <util/CBitArray.h>

#include <utility>

using namespace std;
namespace sf1r
{

DUP_ALG DupDetector::_alg=CHARIKAR;
bool DupDetector::isFirstInstance=true;

DupDetector::DupDetector(unsigned int collectionId, float threshold, const char* filenm)
{

    _collectionId=collectionId;

    nm_ = filenm;

    ndAlgo = NULL;
    _needAnalyze=true;

    char buf[256];
    sprintf(buf, "%s%d", filenm, collectionId);
    fp_index_ = new DupIndex(buf, 1);
    nm_ = buf;
}

DupDetector::DupDetector(unsigned int collectionId, const char* filenm)
{
    _collectionId=collectionId;
    //ndAlgo=new BroderAlgorithm();

    ndAlgo = NULL;
    _needAnalyze=true;

    char buf[256];
    sprintf(buf, "%s%d", filenm, collectionId);
    fp_index_ = new DupIndex(buf, 1);
    nm_ = buf;
}

DupDetector::~DupDetector()
{
    if (ndAlgo)
        delete ndAlgo;
    if (fp_index_) delete fp_index_;
}


bool DupDetector::runDuplicateDetectionAnalysis()
{
    if (ndAlgo)
        delete ndAlgo;
    ndAlgo = NULL;
    read_write_mutex_.lock_shared();
    if (fp_index_)
    {
        fp_index_->indexing(doc_num_);
        fp_index_->flush();
    }
    read_write_mutex_.unlock_shared();
    _needAnalyze=false;
    return true;
}

/**
 * Get a duplicated document list to a given document.
 * @return if no dupDocs for this docId, return false, else return false;
 */
bool DupDetector::getDuplicatedDocIdList(unsigned int docId, std::vector<unsigned int>& docIdList)
{
    read_write_mutex_.lock_shared();
    const Vector32& p = fp_index_->find(docId) ;
    read_write_mutex_.unlock_shared();
    docIdList = std::vector<unsigned int>(p.length());
    memcpy(docIdList.data(), p.data(), p.size());

    return true;
}

/// Get a unique document id list with duplicated ones excluded.
bool DupDetector::getUniqueDocIdList(std::vector<unsigned int>& docIdList)
{
    ///Get all the dup docs of the collection.
    for (size_t i=0; i<docIdList.size(); ++i)
    {
        for (size_t j=i+1; j<docIdList.size(); ++j)
        {
            read_write_mutex_.lock_shared();
            if (fp_index_->is_duplicated(docIdList[i], docIdList[j]))
            {
                docIdList.erase(docIdList.begin()+j);
                --j;
            }
            read_write_mutex_.unlock_shared();
        }
    }

    return true;
}

/// Tell two documents belonging to the same collections are duplicated.
bool DupDetector::isDuplicated( unsigned int docId1, unsigned int docId2)
{
    read_write_mutex_.lock_shared();
    bool ret= fp_index_->is_duplicated(docId1, docId2);
    read_write_mutex_.unlock_shared();
    return ret;
}


/// Insert new documents to a collection. If a document already exists, it is ignored.

bool DupDetector::insertDocuments(const std::vector<unsigned int>& docIdList, const std::vector<std::vector<unsigned int> >& documentList)
{
    IASSERT(docIdList.size() == documentList.size());
    read_write_mutex_.lock_shared();
    fp_index_->ready_for_insert();
    read_write_mutex_.unlock_shared();

    for (size_t i=0;i<docIdList.size();i++)
    {
        //if(!fp_index_->exist(docIdList[i]))
        {
            std::cout<<documentList[i].size();
            izenelib::util::CBitArray bitArray;
            ndAlgo->generate_document_signature(documentList[i], bitArray);
            read_write_mutex_.lock_shared();
            fp_index_->add_doc(docIdList[i], VectorFP((const char*)bitArray.GetBuffer(), bitArray.GetLength()));
            read_write_mutex_.unlock_shared();
        }
    }

    _needAnalyze = true;
    return true;

}

bool DupDetector::insertDocument(uint32_t docid, const std::vector<std::string >& term_list)
{
    if (fp_index_->exist(docid))
        return false;

    izenelib::util::CBitArray bitArray;
    //  CharikarAlgorithm algo;
    ndAlgo->generate_document_signature(term_list, bitArray);
//     std::cout<<"[DUPD] "<<docid<<",";
//     bitArray.display(std::cout);
//     std::cout<<std::endl;

    read_write_mutex_.lock_shared();
    fp_index_->add_doc(docid, VectorFP((const char*)bitArray.GetBuffer(), bitArray.GetLength()));
    read_write_mutex_.unlock_shared();

    return true;

}

/// Update contents of documents.
bool DupDetector::updateDocument(uint32_t docid, const std::vector<std::string >& term_list)
{
    if (!fp_index_->exist(docid))
        return false;

    izenelib::util::CBitArray bitArray;
    if (ndAlgo == NULL)
        ndAlgo = new CharikarAlgo(nm_.c_str());

    ndAlgo->generate_document_signature(term_list, bitArray);


    read_write_mutex_.lock_shared();
    fp_index_->update_doc(docid, VectorFP((const char*)bitArray.GetBuffer(), bitArray.GetLength()));
    read_write_mutex_.unlock_shared();
    return true;

}

/// Remove documents from a collection. From now, those documents should be excluded in the result.
bool DupDetector::removeDocument(unsigned int docid)
{
    if (!fp_index_->exist(docid))
        return false;

    read_write_mutex_.lock_shared();
    fp_index_->del_doc(docid);
    read_write_mutex_.unlock_shared();
    return true;
}

bool DupDetector::release()
{
    return true;
}

bool DupDetector::clear()
{
    return true;
}

/**
 * In following situations, for example, we need to analyze again.
 * If runDuplicateDetectionAnalysis() was not called or failed.
 * After runDuplicateDetectionAnalysis() finished, new data was inserted or updated. (or deleted)
 */
bool DupDetector::needToAnalyze()
{
    return _needAnalyze;
}

unsigned int DupDetector::getCollectionId() const
{
    return _collectionId;
}

}
