///
/// @file ontology-docs.h
/// @brief ontology docs storage class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-09
///

#ifndef SF1R_ONTOLOGY_DOCS_H_
#define SF1R_ONTOLOGY_DOCS_H_

#include <am/tokyo_cabinet/tc_hash.h>

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>

#include <common/type_defs.h>
#include <common/ResultType.h>
#include "faceted_types.h"
#include "ontology_docs_op.h"
NS_FACETED_BEGIN


class OntologyDocs
{

public:
    typedef std::list<uint32_t> DocidListType;
    typedef izenelib::am::tc_hash<CategoryIdType, DocidListType> Cid2DocType;
    typedef std::list<CategoryIdType> CidListType;
    OntologyDocs();
    ~OntologyDocs();

    bool Open(const std::string& dir);

    bool IsOpen() const;

//   bool CopyFrom(const std::string& from_dir, const std::string& to_dir);

    bool InsertDoc(CategoryIdType cid, uint32_t doc_id);

    bool DelDoc(CategoryIdType cid, uint32_t doc_id);

    bool DelCategory(CategoryIdType id);

    bool ApplyModification();

    bool SetDocSupport(CategoryIdType id, const DocidListType& docid_list);

    bool GetDocSupport(CategoryIdType id, DocidListType& docid_list);

    bool GetCategories(uint32_t docid, CidListType& cid_list);

    template <class T>
    static void SortAndUnique(std::vector<T>& t)
    {
        std::sort( t.begin(), t.end() );
        t.erase( std::unique( t.begin(), t.end() ), t.end() );
    }

    template <class T>
    static void SortAndUnique(std::list<T>& t)
    {
        t.sort();
        t.erase( std::unique( t.begin(), t.end() ), t.end() );
    }

private:
    void ProcessOnDoc_(const std::vector<OntologyDocsOp>& changes_on_doc);
    void ProcessOnCategory_(const std::vector<OntologyDocsOp>& changes_on_category);




private:
    bool is_open_;
    std::string dir_;
    Cid2DocType* cid2doc_;
    std::vector<CidListType> doc2cid_;
    std::vector<OntologyDocsOp> changes_;


};
NS_FACETED_END
#endif
