/**
 * @file core/mining-manager/duplicate-detection-submanager/DupDetector.h
 * @author Jinglei
 * @date Created <2009-1-19 13:49:09>
 */

#ifndef DUPDETECTOR_H_
#define DUPDETECTOR_H_

#include "DupTypes.h"
#include <string>
#include <vector>
#include <ir/dup_det/index_fp.hpp>
#include <ir/dup_det/integer_dyn_array.hpp>
#include "charikar_algo.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
//#include <wiselib/IntTypes.h>

namespace sf1r{

/**
 * @brief The interface of duplicate detection.
 * It manages all the offline and online algorithms in duplication detection which
 * aims to eliminate the duplicate and near-duplicate documents from the collection and
 * search result.
  */
class DupDetector {
  typedef izenelib::am::IntegerDynArray<uint64_t> Vector64;
  typedef izenelib::am::IntegerDynArray<uint32_t> Vector32;
  
  typedef uint32_t FP_TYPE;
  typedef izenelib::ir::FpIndex<1, FP_TYPE, 12> DupIndex;
  typedef izenelib::am::IntegerDynArray<FP_TYPE> VectorFP;
  
public:
	DupDetector(unsigned int collectionId, float threshold, const char* filenm="./dupd");
	DupDetector(unsigned int collectionId, const char* filenm="./dupd");
	~DupDetector();
    
	/**
	 * @brief Perform an initialization process and load necessary data.
	 */
	//bool init();

	/**
	 * @brief Get a unique document id list with duplicated ones excluded.
	 */
	bool getUniqueDocIdList(std::vector<unsigned int>& docIdList);

    /**
     * @brief Get a duplicated document list to a given document.
     */
	bool getDuplicatedDocIdList(unsigned int docId, std::vector<unsigned int>& docIdList);

    /**
     * @brief Tell two documents belonging to the same collections are duplicated.
     */
	bool isDuplicated( unsigned int docId1, unsigned int docId2);

    /**
     * @brief Conduct a duplicate detection analysis for a given collection.
     * After this function succeeds, we should be able to get intended results in getUniqueDocIdList(),
     * getDuplicatedDocIdList() and isDuplicated().
     */
	bool runDuplicateDetectionAnalysis();

    /**
     * @brief Whether need to analyze the collection.
     */
	bool needToAnalyze();

    /**
     * @brief Insert new documents to a collection
     */
	bool insertDocuments(const std::vector<unsigned int>& docIdList,
	        const std::vector<std::vector<unsigned int> >& documentList);

    /**
     * @brief Insert new documents to a collection. If a document already exists, it is ignored.
     */
	bool insertDocument(uint32_t docid, const std::vector<std::string >& term_list);

    /**
     * @brief Update contents of documents.
     */
	bool updateDocument(uint32_t docid, const std::vector<std::string >& term_list);

    /**
     * @brief Remove documents from a collection. From now, those documents should be excluded in the result.
     */
	bool removeDocument(unsigned int docid);

    /**
     * @brief Get the collection id for which this detector is running;
     */
	unsigned int getCollectionId() const;

    /**
     * @brief prepare for insert operation.
     */
    inline void ready_for_insert()
    {
      if (fp_index_)
        fp_index_->ready_for_insert();

      doc_num_ = fp_index_->doc_num();

      if (ndAlgo == NULL)
        ndAlgo = new CharikarAlgo(nm_.c_str());
    }
    
    /**
     * @brief Release allocated resource, including the  _docBitMap for the whole collection.
     */
    bool release();

    /**
     * @brief Release all other resources except the  _docBitMap for the whole collection.
     */
    bool clear();

 protected:

    
public:
	/**
	 * @brief The parameter, the algorithm to use, bloom filter could not be used on termId representation of a doc.
	 * BRODER=0, CHARIKAR=1.
	 */
	static DUP_ALG _alg;

private:
	static bool isFirstInstance;

protected:

	/**
	 * @brief the fingerprint index.
	 */
    DupIndex* fp_index_;

    /**
     * @brief whether the collection needs to be analyzed.
     */
	bool _needAnalyze;

	/**
	 * @brief the near duplicate detection algorithm chosen.
	 */
	CharikarAlgo* ndAlgo;

	/**
	 * @brief the number of documents
	 */
    uint32_t doc_num_;

    /**
     * @brief the collectionID to run dup detection.
     */
   unsigned int _collectionId;
   std::string nm_;

   boost::shared_mutex read_write_mutex_;

};

}

#endif /* DUPDETECTOR_H_ */
