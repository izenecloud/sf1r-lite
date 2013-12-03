#include "ontology_manager.h"
#include "counting_trie.h"
#include "JMSmoother.h"
#include "TwoStageSmoother.h"
#include "ontology.h"

#include <mining-manager/taxonomy-generation-submanager/LabelManager.h>

#include <document-manager/DocumentManager.h>
#include <search-manager/ANDDocumentIterator.h>
#include <search-manager/ORDocumentIterator.h>
#include <search-manager/TermDocumentIterator.h>
#include <la-manager/LAManager.h>
#include <la/common/Term.h>

#include <ir/index_manager/index/LAInput.h>
#include <idmlib/util/idm_analyzer.h>

#include <boost/unordered_map.hpp>
#include <algorithm>

using namespace sf1r::faceted;

bool myfunction (std::pair<uint32_t, uint32_t> p1,std::pair<uint32_t, uint32_t> p2)
{
    return p1.second > p2.second;
}

//used for jia's baseline
OntologyManager::OntologyManager(
        const std::string& container,
        const boost::shared_ptr<DocumentManager>& document_manager,
        const std::vector<std::string>& properties,
        idmlib::util::IDMAnalyzer* analyzer)
    : container_(container)
    , document_manager_(document_manager)
    , analyzer_(analyzer)
    , service_(NULL)
    , searcher_(new OntologySearcher())
    , manmade_(new ManmadeDocCategory(container_+"/manmade"))
    , max_docid_file_(container_+"/max_id")
    , docItemListFile_(container_+"/label/docItemList")
{
    for (uint32_t i=0;i<properties.size();i++)
    {
        faceted_properties_.insert( properties[i], 0);
    }
}

OntologyManager::~OntologyManager()
{
    if (service_!=NULL) delete service_;
    delete searcher_;
    delete manmade_;
}

bool OntologyManager::Open()
{
    if (!manmade_->Open())
    {
        std::cerr<<"error when open manmade categories"<<std::endl;
        return false;
    }
    /* if(!reader_->isOpen())
     {
         reader_->open();
     }*/
    /* if(!labelManager_->isOpen())
     {
         labelManager_->open();
     }*/
    return true;
}

bool OntologyManager::close()
{
    //if (reader_->isOpen())
        //reader_->close();
    if (labelManager_->isOpen())
        labelManager_->close();
    return true;
}

/*
 * calculate the total count of terms in one document
 * @param doc : one document
 * @return the total count of term in this document
 */
int OntologyManager::getDocTermsCount(Document& doc)
{
    std::cout<<"OntologyManager::getDocTermsCount"<<std::endl;
    uint32_t doc_terms_cnt = 0;
    Document::property_iterator property_it =doc.findProperty("Content");
    Document::doc_prop_value_strtype content = property_it->second.getPropertyStrValue();

    std::vector<idmlib::util::IDMTerm> term_list;
#ifdef DEBUG_ONTOLOTY
    std::string str ="";
    content.convertString(str,UString::UTF_8);
    cout<<"content is "<<str<<endl;
#endif
    analyzer_->GetStemTermList( propstr_to_ustr(content), term_list );
    //analyzer_->GetTermList(content, term_list);
    doc_terms_cnt+=term_list.size();
#ifdef DEBUG_ONTOLOTY
    cout<<"getDocTermsCount is "<<doc_terms_cnt<<endl;
#endif

    return doc_terms_cnt;
}

std::map<std::pair<uint32_t, uint32_t>, int> coOccurence;

int OntologyManager::getCoOccurence(uint32_t labelTermId,uint32_t topicTermId)
{
    std::pair<uint32_t, uint32_t> label_topic(labelTermId,topicTermId);
    std::map<std::pair<uint32_t, uint32_t>, int>::iterator iter;
    iter =coOccurence.find(label_topic);
    if (iter!=coOccurence.end())
        return iter->second;
    else
        return 0;
}

//std::map<std::pair<uint32_t,izene_termid_t>, uint32_t> docLabelTF;
//std::map<std::pair<uint32_t,uint32_t>, uint32_t> docLabelTF;
std::map<uint32_t,uint32_t> labelDF;

void OntologyManager::calculateCoOccurenceBySearchManager(Ontology* ontology)
{
#if 0
    std::cout<<"OntologyManager::calculateCoOccurenceBySearchManager"<<std::endl;
    izenelib::util::ClockTimer timer;
    IndexReader* pIndexReader=0;
    TermReader* pTermReader = 0;
    if (index_manager_)
    {
        pIndexReader = index_manager_->getIndexReader();
        pTermReader = pIndexReader->getTermReader(1);

#ifdef ONTOLOGY
        cout<<"open pTermReader successfully"<<endl;
#endif
    }
    else
    {
        std::cerr<<"open index_manager failed"<<endl;
        return;
    }
    int maxRuleId = 0;
    for (CategoryIdType cid = 0; cid<ontology->GetCidCount(); cid++)
    {
        if (!ontology->IsLeafCategory(cid))
        {
            continue;
        }

        OntologyNodeRule rule;
        if ( !ontology->GetCategoryRule(cid, rule) )
        {
            std::cout<<"No rule for cid: "<<cid<<std::endl;
            continue;
        }

        for (uint32_t l=0;l<rule.labels.size();l++)
        {
            maxRuleId ++;
        }
    }


    std::vector<std::list<uint32_t> >  topic2DocList;
    if ( docItemListFile_.Load() )
    {
        topic2DocList = docItemListFile_.GetValue();
#ifdef ONTOLOGY
        cout<<"load kpe result successfully, and label2DocList size is "<<topic2DocList.size()<<endl;
#endif
    }
    uint32_t maxTopics = topic2DocList.size() >50000? 50000:topic2DocList.size();
//     labelManager_->open();
    AnalysisInfo analysisInfo;
    analysisInfo.analyzerId_ = "la_sia";
    analysisInfo.tokenizerNameList_.insert("tok_divide");
    analysisInfo.tokenizerNameList_.insert("tok_unite");

    uint32_t ruleId =0;
    for (CategoryIdType cid = 0; cid<ontology->GetCidCount(); cid++)
    {
        if (!ontology->IsLeafCategory(cid))
        {
            continue;
        }
        OntologyNodeRule rule;
        if ( !ontology->GetCategoryRule(cid, rule) )
        {
            std::cout<<"No rule for cid: "<<cid<<std::endl;
            continue;
        }
        cout<<"rule labels size is "<<rule.labels.size()<<endl;
        for (uint32_t l=0;l<rule.labels.size();l++,ruleId++)
        {
            izenelib::util::ClockTimer timer1;
            izenelib::util::UString ruleLabel = rule.labels[l];

            //get tokenized result
            TermIdList termIdList;
            laManager_->getTermIdList(idManager_.get(),
                                      ruleLabel,
                                      analysisInfo,
                                      termIdList);
#ifdef ONTOLOGY
            cout<<"termIdList is "<<termIdList.size()<<endl;
#endif
            // cout<<"get termidlist cost time "<<timer1.elapsed()<<endl;
            izenelib::util::ClockTimer timer2;
            DocumentIterator* pORDocIterator = new ORDocumentIterator();
            for (uint32_t i=0;i<property_ids_.size();i++)
            {
                DocumentIterator* pAndDocIterator = new ANDDocumentIterator();

                for (TermIdList::iterator p = termIdList.begin();
                        p != termIdList.end(); p++)
                {
                    TermDocumentIterator* pTermDocIterator =
                        new TermDocumentIterator(
                        //termid,
                        (*p).termid_,
                        collectionId_,
                        pIndexReader,
                        //"Content",
                        property_names_[i],
                        //1,
                        property_ids_[i],
                        1,
                        false);
                    if (pTermDocIterator->accept())
                    {
                        pAndDocIterator->add(pTermDocIterator);
                    }
                    //delete pTermDocIterator;//can't delete why?
                }
                pORDocIterator->add(pAndDocIterator);

            }

            // cout<<"get pORDocIterator cost time "<<timer2.elapsed()<<endl;




            pORDocIterator->skipTo(maxDocid_);
            izenelib::util::ClockTimer timer3;
            for (uint32_t i=0;i<maxTopics;i++)
            {
                // for(int i=0;i<10;i++){

                std::list<uint32_t> docList = topic2DocList[i];
                std::pair<uint32_t, uint32_t>label_topic(ruleId,i+1);
                std::list<uint32_t>::iterator iter= docList.begin();
                while (pORDocIterator->next())
                {

                    const unsigned int docid =pORDocIterator->doc();
                    unsigned int tf = pORDocIterator->tf();
                    /*std::string labelStr;
                    ruleLabel.convertString(labelStr,UString::UTF_8);
                    cout<<"docid is "<<docid<<", label is "<<labelStr<<",tf is "<<tf<<endl;*/



//                  std::pair<uint32_t,uint32_t> doc_label(docid, ruleId);
//                  docLabelTF[doc_label] = tf;

                    std::vector<unsigned int> vectfs;
                    if (docRuleTFSDB_->getValue(docid,vectfs)==false)
                    {
                        vectfs.resize(maxRuleId);
                    }
                    vectfs[ruleId] = tf;
                    docRuleTFSDB_->update(docid,vectfs);



                    while (iter !=docList.end())
                    {
                        if (*iter <docid)
                            iter = docList.erase(iter);
                        else if (*iter == docid)
                        {
                            coOccurence[label_topic]++;
                            //cout<<"target is "<<docid<<endl;
                            iter = docList.erase(iter);
                        }
                        else
                            break;

                    }


                }
#ifdef ONTOLOGY_DEBUG
                if (coOccurence.find(label_topic)->second>0)
                {

                    std::string labelStr;
                    ruleLabel.convertString(labelStr,UString::UTF_8);
                    UString topic ;
                    labelManager_->getLabelStringByLabelId(i+1, topic);
                    std::string topicStr;
                    topic.convertString(topicStr,UString::UTF_8);
                    cout<<"user-defined label is "<<labelStr;
                    cout<<",topic is "<<topicStr<<"cooccurence is "<<coOccurence.find(label_topic)->second<<endl;
                }

#endif
            }
            //cout<<"tranverse topics cost time "<<timer3.elapsed()<<endl;
        }
    }


    /* for(int i=0;i<topic2DocList.size();i++){
        std::list<uint32_t> docList = topic2DocList[i];
        labelDF[i+1] = docList.size();
     }
    */
    cout<<"calculate cooccurence cost time is "<<timer.elapsed()<<endl;
    delete pTermReader;

#endif
}

bool OntologyManager::SetXML(const std::string& xml)
{
    boost::lock_guard<boost::mutex> lock(mutex_);
    return SetXMLSimple_(xml);
}

bool OntologyManager::ProcessCollection(bool rebuild)
{
    boost::lock_guard<boost::mutex> lock(mutex_);
//     return ProcessCollectionWithSmooth_(rebuild);
    return ProcessCollectionSimple_(rebuild);
}

bool OntologyManager::ProcessCollectionSimple_(bool rebuild)
{
    //do on documents
    MEMLOG("[Mining] FACETED starting, rebuild=%d",rebuild);
    Ontology* ontology = service_;
    if (rebuild) ontology = edit_;
    if (ontology==NULL)
    {
        std::cerr<<"No ontology available."<<std::endl;
        return false;
    }

    //build rules
    typedef std::pair<CategoryIdType, OntologyNodeRuleInt > IntRule;
    std::vector<IntRule> rules;
    {
        std::cout<<"cid count: "<<ontology->GetCidCount()<<std::endl;
        for (CategoryIdType cid = 0; cid<ontology->GetCidCount(); cid++)
        {
            OntologyNodeRule rule;
            if ( !ontology->GetCategoryRule(cid, rule) )
            {
                std::cout<<"No rule for cid: "<<cid<<std::endl;
                continue;
            }
//       std::cout<<"LC:"<<rule.labels.size()<<std::endl;
            if (!ontology->IsLeafCategory(cid))
            {
                continue;
            }
            OntologyNodeRuleInt int_rule;
//             std::vector<std::vector<uint32_t> > intrule;
            for (uint32_t l=0;l<rule.labels.size();l++)
            {
                std::vector<idmlib::util::IDMTerm> term_list;
                analyzer_->GetStemTermList( rule.labels[l], term_list );
                uint32_t last_position=0;
                std::vector<uint32_t> terms;
                izenelib::util::UString term_text;
                for (uint32_t u=0;u<term_list.size();u++)
                {
                    if (u>0&&term_list[u].position>last_position+1)
                    {
                        if (!terms.empty())
                        {
                            int_rule.labels_int.push_back(terms);
                            int_rule.labels.push_back(term_text);
                            terms.resize(0);
                            term_text = izenelib::util::UString("", izenelib::util::UString::UTF_8);
                        }
                    }
                    terms.push_back(term_list[u].id);
                    term_text += term_list[u].text;
                    last_position = term_list[u].position;
                }
                if (!terms.empty())
                {
                    int_rule.labels_int.push_back(terms);
                    int_rule.labels.push_back(term_text);
                }
            }
//             std::cout<<"rule_info "<<cid<<","<<int_rule.labels_int.size()<<std::endl;
            rules.push_back(std::make_pair(cid, int_rule));

        }
    }
    if (rules.empty())
    {
        std::cerr<<"No rules available."<<std::endl;
        return false;
    }
    //get predefined categories
    //TODO
    izenelib::am::rde_hash<izenelib::util::UString, CategoryIdType> manmade_categories;
    std::vector<ManmadeDocCategoryItem> items;
    if (!manmade_->Get(items))
    {
        std::cerr<<"Get manmade categories failed"<<std::endl;
    }
    else
    {
        if (ontology->GenCategoryId(items))
        {
            for (uint32_t i=0;i<items.size();i++)
            {
                if (items[i].cid!=0)
                {
                    manmade_categories.insert(items[i].str_docid, items[i].cid);
                }
            }
        }
        else
        {
            std::cerr<<"generate category id for manmade failed"<<std::endl;
        }
    }


    uint32_t max_docid = 0;
    if (!rebuild)
    {
        max_docid = GetProcessedMaxId_();
    }
    uint32_t process_count = 0;
    Document doc;
    std::cout<<"Process Collection: Will processing from "<<max_docid+1<<" to "<<document_manager_->getMaxDocId()<<std::endl;
    //for( uint32_t docid = max_docid+1; docid<=10000; docid++)
    for ( uint32_t docid = max_docid+1; docid<=document_manager_->getMaxDocId(); docid++)
    {
        process_count++;
        if ( process_count %1000 == 0 )
        {
            MEMLOG("[FACETED] inserted %d. docid: %d", process_count, docid);
        }
        bool b = document_manager_->getDocument(docid, doc);
        if (!b) continue;
        document_manager_->getRTypePropertiesForDocument(docid, doc);
//     std::cout<<"Processing docid: "<<docid<<std::endl;
        Document::property_iterator property_it = doc.findProperty("DOCID");
        if (property_it!=doc.propertyEnd())
        {
            Document::doc_prop_value_strtype str_docid = property_it->second.getPropertyStrValue();
            CategoryIdType* cid = manmade_categories.find(propstr_to_ustr(str_docid));
            if (cid!=NULL)
            {
                std::cout<<"Manmade category: "<<docid<<" -> "<<*cid<<std::endl;
                ontology->InsertDoc(*cid, docid);
                continue;
            }
        }
        uint32_t terms_count = 0;
        CountingTrie trie;
        property_it = doc.propertyBegin();
        Document::doc_prop_value_strtype last_faceted_property;
        while (property_it != doc.propertyEnd())
        {
            if (faceted_properties_.find(property_it->first))
            {
                const Document::doc_prop_value_strtype& content = property_it->second.getPropertyStrValue();
                std::vector<idmlib::util::IDMTerm> term_list;
                analyzer_->GetStemTermList( propstr_to_ustr(content), term_list );
//              std::cout<<"after la "<<term_list.size()<<std::endl;
//              if(term_list.size()>100000)
//              {
//                  std::cout<<content<<std::endl;
//              }
                terms_count+=term_list.size();
                uint32_t last_position=0;
                std::vector<uint32_t> terms;
                uint32_t max_term = std::min((uint32_t)(term_list.size()), (uint32_t)1000);
                for (uint32_t u=0;u<max_term;u++)
                {
                    if (u>0&&term_list[u].position>last_position+1)
                    {
                        trie.Append(terms);
                        terms.resize(0);
                    }
                    terms.push_back(term_list[u].id);
                    last_position = term_list[u].position;
                }
                trie.Append(terms);
                last_faceted_property = content;
            }
            property_it++;
        }
        std::vector<std::pair<double,CategoryIdType> > score_item;
        for (uint32_t r=0;r<rules.size();r++)
        {
            CategoryIdType cid = rules[r].first;
            double score = 0.0;
            const OntologyNodeRuleInt& int_rule = rules[r].second;
            for (uint32_t l=0;l<int_rule.labels_int.size();l++)
            {
                uint32_t count = trie.Count(int_rule.labels_int[l]);
                if(count>0)
                {
//                     score += (double)count/terms_count;
                    score += (double)count;
#ifdef SIMPLE_DEBUG
                    std::string str;
                    int_rule.labels[l].convertString(str, izenelib::util::UString::UTF_8);
                    std::cout<<"matched "<<str<<std::endl;
#endif
                }
            }
#ifdef SIMPLE_DEBUG
            std::cout<<"cid "<<cid<<" matches "<<(uint32_t)score<<std::endl;
#endif
            score_item.push_back(std::make_pair(score, cid));
//       std::cout<<"{"<<docid<<","<<cid<<","<<score<<"}"<<std::endl;
        }
        if (score_item.size()>0)
        {
            std::sort(score_item.begin(), score_item.end(), std::greater<std::pair<double,CategoryIdType> >());
            CategoryIdType c = score_item[0].second;
            if ( score_item[0].first>0.0)
            {
                std::string t = propstr_to_str(last_faceted_property);
                izenelib::util::UString cn;
                ontology->GetCategoryName(c, cn);
                std::string cns;
                cn.convertString(cns, izenelib::util::UString::UTF_8);
                std::cout<<"[Title] "<<t<<std::endl;
                std::cout<<"[Category] "<<cns<<std::endl;
                ontology->InsertDoc(c, docid);
            }
        }


    }
    if (! ontology->ApplyModification())
    {
        std::cerr<<"[FACETED] apply modification failed."<<std::endl;
        return false;
    }
    max_docid_file_.SetValue(document_manager_->getMaxDocId());
    max_docid_file_.Save();

    MEMLOG("[Mining] FACETED finished.");
    OutputToFile_(container_+"/text_output.txt", ontology);
    return true;
}




bool OntologyManager::GetXML(std::string& xml)
{
    if (service_==NULL) return false;
    return service_->GetXML(xml);
}

bool OntologyManager::SetXMLSimple_(const std::string& xml)
{
    std::string edit_dir = container_+"/edit";
    std::string service_dir = container_+"/service";
    try
    {
        boost::filesystem::remove_all(edit_dir);
    }
    catch (std::exception& ex)
    {
        std::cerr<<"boost error "<<ex.what()<<std::endl;
        return false;
    }

    Ontology* edit = new Ontology();

    if (!edit->InitOrLoad(edit_dir))
    {
        std::cerr<<"Ontology InitOrLoad failed"<<std::endl;
        delete edit;
        return false;
    }
    if (!edit->SetXML(xml))
    {
        std::cerr<<"Ontology SetXML failed"<<std::endl;
        delete edit;
        return false;
    }
    //do not copy docs from service, just rebuild
//   if( service_!=NULL)
//   {
//     if(!edit->CopyDocsFrom(service_))
//     {
//       std::cerr<<"Ontology CopyDocsFrom failed"<<std::endl;
//       delete edit;
//       return false;
//     }
//   }

    //TODO Rebuild the collection on edit
    edit_ = edit;

    if (ProcessCollectionSimple_(true))
    {
        delete edit;
        if (service_!=NULL) delete service_;
        try
        {
            boost::filesystem::remove_all(service_dir);
            boost::filesystem::rename(edit_dir, service_dir);
        }
        catch (std::exception& ex)
        {
            std::cerr<<"service error: "<<ex.what()<<std::endl;
            return false;
        }
        service_ = new Ontology();
        if (!service_->Load(service_dir))
        {
            std::cerr<<"Ontology service load failed"<<std::endl;
            return false;
        }

        searcher_->SetOntology(service_);
    }
    else
    {
        std::cerr<<"Process collection on new ontology failed"<<std::endl;
        delete edit;
        try
        {
            boost::filesystem::remove_all(edit_dir);
        }
        catch (std::exception& ex)
        {
            std::cerr<<"boost remove_all error"<<ex.what()<<std::endl;
        }
        return false;
    }
    return true;
}

bool OntologyManager::DefineDocCategory(const std::vector<ManmadeDocCategoryItem>& items)
{
//   std::vector<ManmadeDocCategoryItem>::const_iterator it = items.begin();
//   std::vector<ManmadeDocCategory::ItemType> insert_items
//   while(it!=items.end())
//   {
//     const std::pair<uint32_t, CategoryNameType>& item = *it;
//     ++it;
//
//   }


    std::vector<ManmadeDocCategoryItem> input(items);
    //sort and unique items
//     std::vector<ManmadeDocCategoryItem> sorted(items);
//     std::stable_sort(sorted.begin(), sorted.end(), ManmadeDocCategoryItem::CompareStrDocId );
//
//     izenelib::util::UString last_str_docid;
//     for (uint32_t i=0;i<sorted.size();i++)
//     {
//         if (sorted[i].str_docid!=last_str_docid&&i>0)
//         {
//             if (sorted[i-1].ValidInput())
//                 input.push_back(sorted[i-1]);
//         }
//         last_str_docid = sorted[i].str_docid;
//     }
//     if (sorted.back().ValidInput())
//         input.push_back(sorted.back());
    if (input.empty())
    {
        std::cerr<<"No valid input data"<<std::endl;
        return false;
    }
    if (!manmade_->Add(input))
    {
        std::cerr<<"Add man made doc category failed."<<std::endl;
        return false;
    }
    //TODO move the doc-cat in currect service
    if (service_!=NULL)
    {
        for (uint32_t i=0;i<input.size();i++)
        {
            Ontology::CidListType cid_list;
            if (service_->GetCategories(input[i].docid, cid_list))
            {
                Ontology::CidListType::iterator it = cid_list.begin();
                while (it!=cid_list.end())
                {
                    CategoryIdType cid = *it;
                    it++;
                    service_->DelDoc(cid, input[i].docid);
                }
            }
            service_->InsertDoc(input[i].cid, input[i].docid);
        }
        service_->ApplyModification();
    }
    return true;
}

uint32_t OntologyManager::GetProcessedMaxId_()
{
    if ( max_docid_file_.Load() )
    {
        return max_docid_file_.GetValue();
    }
    return 0;
}

void OntologyManager::OutputToFile_(const std::string& file, Ontology* ontology)
{
    std::ofstream ofs(file.c_str());
    Document doc;

    for ( uint32_t docid = 1; docid<=document_manager_->getMaxDocId(); docid++)
    {

        if ( docid %1000 == 0 )
        {
            MEMLOG("[FACETED-OUTPUT] docid: %d", docid);
        }
        bool b = document_manager_->getDocument(docid, doc);
        if (!b) continue;
//     std::cout<<"Processing docid: "<<docid<<std::endl;
        Document::property_iterator property_it = doc.findProperty("DOCID");
        if (property_it!=doc.propertyEnd())
        {
            Document::doc_prop_value_strtype str_docid = property_it->second.getPropertyStrValue();
            std::string s_docid = propstr_to_str(str_docid);
            std::list<uint32_t> cid_list;
            ontology->GetCategories(docid, cid_list);
            if(cid_list.size()==0) continue;
            ofs<<s_docid<<"\t";
            std::list<uint32_t>::iterator it = cid_list.begin();
            bool first = true;
            while(it!=cid_list.end())
            {
                uint32_t cid = *it;
                ++it;
                izenelib::util::UString name;
                if(!ontology->GetCategoryName(cid, name)) continue;
                std::string s_name;
                name.convertString(s_name, izenelib::util::UString::UTF_8);
                if(first)
                {
                    first = false;
                }
                else
                {
                    ofs<<",";
                }
                ofs<<s_name;
            }
            ofs<<std::endl;
        }
    }
    ofs.close();
}
