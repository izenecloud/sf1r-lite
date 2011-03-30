///
/// @file ontology.h
/// @brief ontology representation class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-08
///

#ifndef SF1R_ONTOLOGY_H_
#define SF1R_ONTOLOGY_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"
#include "ontology_nodes.h"
#include "ontology_docs.h"
#include "ontology_rep.h"
#include "manmade_doc_category.h"
#include <util/ticpp/ticpp.h>
NS_FACETED_BEGIN


class Ontology
{
private:
    struct XmlNode
    {
        XmlNode()
        {
        }
        XmlNode(const CategoryNameType& parent, const CategoryNameType& pname)
                :parent_name(parent), name(pname)
        {
        }
        CategoryNameType parent_name;
        CategoryNameType name;
        OntologyNodeRule rule;
    };
public:
    typedef OntologyNodes CategoryStorageType;
    typedef OntologyDocs DocsStorageType;
    typedef DocsStorageType::DocidListType DocidListType;
    typedef DocsStorageType::CidListType CidListType;
    typedef uint8_t DepthType;
    typedef std::pair<CategoryIdType, DepthType> CategoryIdDepth;
    typedef std::pair<DocidListType, DepthType> DocListDepth;

public:
    Ontology();
    ~Ontology();

    bool Load(const std::string& dir);

    bool Init(const std::string& dir);

    bool InitOrLoad(const std::string& dir);

    static bool Copy(const std::string& from_dir, const std::string& to_dir);

    bool CopyDocsFrom(Ontology* ontology);

    bool IsOpen() const;

    bool SetXML(const std::string& xml);

    bool GetXML(std::string& xml);

    bool AddCategoryNode(CategoryIdType parent_id, const CategoryNameType& name, CategoryIdType& id);

    bool RenameCategory(CategoryIdType id, const CategoryNameType& new_name);

    bool MoveCategory(CategoryIdType id, CategoryIdType new_parent_id);

    bool DelCategory(CategoryIdType id);

    bool InsertDoc(CategoryIdType cid, uint32_t doc_id);

    bool DelDoc(CategoryIdType cid, uint32_t doc_id);

    bool ApplyModification();

    std::string GetName() const;

    bool SetName(const std::string& name);

    bool GetRepresentation(OntologyRep& rep, bool include_top = false);

    bool GetDocSupport(CategoryIdType id, DocidListType& docid_list);

    bool GetRecursiveDocSupport(CategoryIdType id, DocidListType& docid_list);

    bool GetDocCount(CategoryIdType id, uint32_t& count) const;

    bool GetCategories(uint32_t docid, CidListType& cid_list);

    bool GetParent(CategoryIdType id, CategoryIdType& parent_id) const;

    bool GetCategoryName(CategoryIdType id, CategoryNameType& name) const;

    bool DocInCategory(uint32_t docid, CategoryIdType cid);

    bool GetCategoryRule(CategoryIdType id, OntologyNodeRule& rule) const;

    bool SetCategoryRule(CategoryIdType id, const OntologyNodeRule& rule);

    bool IsLeafCategory(CategoryIdType id);

    bool GenCategoryId(std::vector<ManmadeDocCategoryItem>& items);

    DocsStorageType* Docs()
    {
        return docs_storage_;
    }

    uint32_t GetCidCount() const
    {
        return category_storage_->GetCidCount();
    }

    inline static CategoryIdType TopCategoryId()
    {
        return CategoryStorageType::TopCategoryId();
    }

    //for test propose
    bool AddCategoryNodeStr(CategoryIdType parent_id, const std::string& name, CategoryIdType& id);

private:
    bool LoadCidDocCount_();
    void SetDocCount_(CategoryIdType id, uint32_t count);

    //iterate
    void ForwardIterate_(const CategoryIdDepth& id_depth, std::stack<CategoryIdDepth>& items);

    void PostIterate_(const CategoryIdType& id, std::queue<CategoryIdType>& items);

    bool GetRepresentation_(const CategoryIdDepth& id_depth, OntologyRep& rep, bool include_top);

    bool GetRecursiveDocSupport_(CategoryIdType id, DocidListType& docid_list);

    bool GetXmlNode_(izenelib::util::ticpp::Element* node, XmlNode& xml_node);


private:
    std::string dir_;
    bool is_open_;
    std::string name_;
    CategoryStorageType* category_storage_;
    DocsStorageType* docs_storage_;
    std::vector<uint32_t> cid_doccount_;
//   RuleStorageType* rule_storage_;

    boost::shared_mutex mutex_;



};
NS_FACETED_END
#endif /* SF1R_ONTOLOGY_H_ */
