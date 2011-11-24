#include <util/functional.h>
#include "LabelManager.h"
#include <mining-manager/concept-id-manager.h>
#include <mining-manager/util/MUtil.hpp>
#include <mining-manager/util/TermUtil.hpp>
#include <mining-manager/util/FSUtil.hpp>
#include <mining-manager/MiningUtil.h>
#include <log-manager/PropertyLabel.h>
#include <util/hashFunction.h>
#include <stdint.h>
#include <algorithm>
#include <am/graph_index/dyn_array.hpp>
#include <common/type_defs.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/serialization/list.hpp>

#define GEN_LABEL_INVERTED_TEXT ;

using namespace sf1r;

LabelManager::LabelManager(const std::string& path, bool no_doc_container) :
        path_(path), isOpen_(false), no_doc_container_(no_doc_container)
        , collection_name_()
        , info_(0)
        , serInfo_(path_+"/ser_info")
        , idManager_(NULL)
        , docLabelInfo_(NULL)
        , sorted_label_(NULL)
        , dfInfo_(NULL)
        , labelStream_()
        , labelMap_(NULL)
        , savedLabelHashPath_(path_+"/labelHash")
        , ldWriter_(new LabelDistributeSSFType::WriterType(path_+"/labeldistribute"))
        , docTableMutex_()
        , cacheHit_(0)
        , cacheNoHit_(0)
        , docItemListFile_(path_+"/docItemList")
//   , sim_(new TermSimilarityType(path_+"/sim"))

{
    RegisterErrorMsg_(1, "file io error");
}

void LabelManager::SetDbSave(const std::string& collection_name)
{
    collection_name_ = collection_name;
}

void LabelManager::open()
{
    if ( isOpen() ) return;

    serInfo_.setCacheSize(1);
    serInfo_.open();
    bool b = serInfo_.get(0, info_);
    if (!b)
    {
        serInfo_.update(0, info_);
    }
    serInfo_.flush();

    docLabelInfo_ = new DocTable<>(path_ + "/doc_label_info");
    sorted_label_ = new izenelib::am::tc_hash<uint32_t, std::vector<uint32_t> >(path_+"/sorted_label");
    sorted_label_->open();
//     docLabelInfo_ = new DocLabelInfoType(path_ + "/doc_label_info");
//     DocLabelInfoType::ContainerType& container = docLabelInfo_->getContainer();
//     container.tune(4*200);
//     docLabelInfo_->setCacheSize(10000000);
//     docLabelInfo_->open();
    boost::filesystem::create_directories(path_+"/concept-id");
    idManager_ = new ConceptIDManager(path_+"/concept-id");
    if ( !idManager_->Open() )
    {
        //to do
    }
    dfInfo_ = new izenelib::am::SequenceFile<uint32_t>(path_+"/dfinfo");
    dfInfo_->setCacheSize(50000000);
    dfInfo_->open();
    typeInfo_ = new izenelib::am::SequenceFile<uint8_t>(path_+"/typeinfo");
    typeInfo_->setCacheSize(50000000);
    typeInfo_->open();
    labelMap_ = new izenelib::am::tc_hash<hash_t,uint32_t >(path_+"/labelmap");
    labelMap_->open();
    std::string labelStreamFile = path_ + "/label.stream";
    labelStream_.open(labelStreamFile.c_str(), std::ios_base::app);
    if (!labelStream_.is_open())
    {
        throw FileOperationException(labelStreamFile+" open", "LabelManager");
    }
    ldWriter_->open();
//     sim_->Open();
    isOpen_ = true;
//     uint8_t type = 0;
//     for(uint32_t lid = 1; lid<getMaxLabelID(); lid++)
//     {
//       if(getLabelType( lid, type))
//       {
//         if(type>0)
//         {
//           izenelib::util::UString text;
//           if(getLabelStringByLabelId(lid, text))
//           {
//             std::string str;
//             text.convertString(str, izenelib::util::UString::UTF_8);
//             std::cout<<"[Find NE] "<<str<<" , "<<(int)type<<std::endl;
//           }
//         }
//       }
//     }
//     std::cout<<"UString size: "<<sizeof(izenelib::util::UString)<<std::endl;
//     std::cout<<"string size: "<<sizeof(std::string)<<std::endl;
//     {
//         izenelib::util::UString labelStr;
//         getLabelStringByLabelId(1, labelStr);
//         std::string str;
//         labelStr.convertString(str, izenelib::util::UString::UTF_8);
//         std::cout<<"Label string 1 : "<<str<<std::endl;
//     }
//     {
//         izenelib::util::UString labelStr;
//         getLabelStringByLabelId(10101, labelStr);
//         std::string str;
//         labelStr.convertString(str, izenelib::util::UString::UTF_8);
//         std::cout<<"Label string 10101 : "<<str<<std::endl;
//     }
}

bool LabelManager::isOpen()
{
    return isOpen_;
}

void LabelManager::AddTask(TaskFunctionType function)
{
    task_list_.push_back(function);
}

void LabelManager::ClearAllTask()
{
    task_list_.clear();
}

void LabelManager::flush()
{
    if (isOpen())
    {
        serInfo_.flush();
        dfInfo_->flush();
        typeInfo_->flush();
        docLabelInfo_->flush();
        sorted_label_->flush();
        idManager_->Flush();
        labelStream_.flush();
//         semHypernym_.flush();
    }

}

void LabelManager::close()
{
    if (isOpen())
    {
        isOpen_ = false;
        serInfo_.close();
        if (docLabelInfo_ != NULL)
        {
//             docLabelInfo_->close();
            delete docLabelInfo_;
            docLabelInfo_=NULL;
        }
        if (sorted_label_ != NULL)
        {
            sorted_label_->close();
            delete sorted_label_;
            sorted_label_=NULL;
        }

        if (idManager_ != NULL)
        {
            idManager_->Close();
            delete idManager_;
            idManager_=NULL;
        }
        if (dfInfo_ != NULL)
        {
            dfInfo_->close();
            delete dfInfo_;
            dfInfo_ = NULL;
        }
        if (typeInfo_ != NULL)
        {
            typeInfo_->close();
            delete typeInfo_;
            typeInfo_ = NULL;
        }
        if (labelMap_ != NULL)
        {
            labelMap_->close();
            delete labelMap_;
            labelMap_ = NULL;
        }
        if (labelStream_.is_open())
        {
            labelStream_.close();
        }
        delete ldWriter_;

    }

}

LabelManager::~LabelManager()
{
    close();
}

void LabelManager::deleteDocLabels(docid_t docId)
{
    if ( !isOpen() ) return;
    std::vector<labelid_t> labelIdList;
    bool ret = getLabelsByDocId(docId, labelIdList);
    if ( ret )
    {
        uint32_t df = 0;
        for (uint32_t i=0;i<labelIdList.size();i++)
        {
            labelid_t labelId = labelIdList[i];
            bool b = getLabelDF(labelId, df);
            if (b)
            {
                if (df>0)
                {
                    setLabelDF( labelId, df-1 );
                }
            }
        }
    }
    docTableMutex_.lock();
    docLabelInfo_->delete_doc(docId);
    docTableMutex_.unlock();

}

void LabelManager::insertLabel(
    const izenelib::util::UString& labelStr,
    const std::vector<id2count_t>& docItemList,
    uint8_t score,
    uint8_t type)
{
    if ( !isOpen() ) return;

//     if( label2DocWriter_ == NULL )
//     {
//
//         label2DocWriter_ = new Label2DocSSFType::WriterType(FSUtil::getTmpFileFullName(path_));
//         label2DocWriter_->open();
//     }
//     uint32_t reverse_df = std::numeric_limits< uint32_t >::max()- (uint32_t)(docItemList.size());
//     label2DocWriter_->append(reverse_df, Label2DocItem(labelStr, docItemList, type, score) );
    label2DocWriter_.push_back(std::make_pair(docItemList.size(), Label2DocItem(labelStr, docItemList, type, score)));

}

void LabelManager::insertLabel(
    docid_t docId,
    const izenelib::util::UString& labelStr)
{
    if ( !isOpen() ) return;
    std::vector<id2count_t> docItemList(1);
    docItemList[0].first = docId;
    docItemList[0].second = 1;

    insertLabel(labelStr, docItemList, 10);
}

bool LabelManager::getLabelType(uint32_t labelId, uint8_t& type)
{
    return typeInfo_->get(labelId, type);
}

bool LabelManager::getLabelsByDocIdList(const std::vector<docid_t>& docIdList, std::vector<std::vector<labelid_t> >& labels)
{
    if ( !isOpen() ) return false;
    std::vector<char* > dataList;
    std::vector<uint16_t> lenList;

    docTableMutex_.lock_shared();
    bool ret = docLabelInfo_->get_doc(docIdList, dataList, lenList);
    docTableMutex_.unlock_shared();

    if (!ret) return ret;

    std::vector<labelid_t> iLabelList;
    for (std::size_t i=0;i<docIdList.size();i++)
    {
        iLabelList.clear();
        if ( dataList[i] != NULL )
        {
            decode_(dataList[i], lenList[i], iLabelList);

            free(dataList[i]);

        }
        labels.push_back(iLabelList);
    }

//     docTableMutex_.lock_shared();
//     bool ret = docLabelInfo_->get<docid_t>(docIdList, labels);
//     docTableMutex_.unlock_shared();

    return ret;
}

bool LabelManager::getLabelsByDocId(docid_t docId, std::vector<labelid_t>& labelIdList)
{
    if ( !isOpen() ) return false;
    std::vector<docid_t> docIdList(1, docId);
    std::vector<std::vector<labelid_t> > labels;
    bool ret = getLabelsByDocIdList(docIdList, labels);
    if (!ret) return false;
    labelIdList.resize(1);
    labelIdList.swap(labels[0]);
    return true;
}

bool LabelManager::getSortedLabelsByDocId(docid_t docId, std::vector<labelid_t>& labelIdList)
{
    return sorted_label_->get(docId, labelIdList);
}

void LabelManager::buildDocLabelMap()
{
    if ( !isOpen() ) return;
    if (label2DocWriter_.empty()) return;
    {
        label2DocWriter_.sort(std::greater<std::pair<uint32_t, Label2DocItem> >());
    }
    docItemListFile_.Load();
    Doc2LabelSSFType::WriterType doc2LabelWriter(FSUtil::getTmpFileFullName(path_));
    doc2LabelWriter.open();
    {
        std::vector<std::list<uint32_t> > label2DocList;
        std::list<std::pair<uint32_t, Label2DocItem> >::const_iterator it = label2DocWriter_.begin();
#ifdef GEN_LABEL_INVERTED_TEXT
        std::ofstream invertofs( (path_+"/inverted_text").c_str() );
#endif
        while ( it!= label2DocWriter_.end())
        {
            Label2DocItem value = it->second;
            ++it;
            izenelib::util::UString str = value.get<0>();
            std::vector<id2count_t> docItemList = value.get<1>();
            uint8_t type = value.get<2>();
            uint8_t score = value.get<3>();
            uint32_t label_id = getLabelId_(str, docItemList.size(), type, score);

            TaskFunctionListType::iterator it = task_list_.begin();
            while ( it!= task_list_.end() )
            {
                (*it)(label_id, str, docItemList, type, score);
                ++it;
            }
#ifdef GEN_LABEL_INVERTED_TEXT
            std::string sss;
            str.convertString(sss, izenelib::util::UString::UTF_8);
            invertofs<<sss<<"\t";
            for (uint32_t m=0;m<docItemList.size();m++)
            {
                if (m>0)
                {
                    invertofs<<"|";
                }
                invertofs<<docItemList[m].first<<","<<docItemList[m].second;
            }
            invertofs<<std::endl;
#endif
//         sim_->Append(label_id, docItemList);
            std::list<uint32_t> docList;

            for (uint32_t m=0;m<docItemList.size();m++)
            {
                docid_t docId = docItemList[m].first;
                docList.push_back(docId);
                if ( docId > info_  )
                {
                    info_ = docId;
                }
                doc2LabelWriter.append(docId, std::make_pair(label_id, docItemList[m].second) );
            }
            label2DocList.push_back(docList);

        }
        docItemListFile_.SetValue(label2DocList);
        docItemListFile_.Save();
        label2DocWriter_.clear();
#ifdef GEN_LABEL_INVERTED_TEXT
        invertofs.close();
#endif
    }
    doc2LabelWriter.close();
    labelMap_->flush();
    serInfo_.update(0, info_);
    serInfo_.flush();
//     if(!sim_->Compute())
//     {
//       std::cout<<"compute sim failed."<<std::endl;
//     }

    {
        Doc2LabelSSFType::SorterType sorter;
        sorter.sort(doc2LabelWriter.getPath());
    }

    Doc2LabelSSFType::ReaderType reader(doc2LabelWriter.getPath());
    reader.open();

    docid_t docId;
    id2count_t labelId2Count;
    std::vector<id2count_t> labelItemList;
    docid_t lastDocId = 0;
    bool isFirst = true;
    uint32_t docCount = 0;
    uint64_t labelCount = 0;
    uint64_t p = 0;
    while ( reader.next(docId, labelId2Count) )
    {

//         std::cout<<"RRRR "<<docId<<" "<<labelId2Count.first<<std::endl;
        if (isFirst)
        {
            labelItemList.push_back(labelId2Count);
            isFirst = false;
        }
        else
        {
            if (docId == lastDocId)
            {

            }
            else
            {
//                 std::cout<<"XXX "<<lastDocId<<" "<<labelItemList.size()<<std::endl;
                insertToDocTable_( lastDocId, labelItemList);
                ++docCount;
                labelCount += labelItemList.size();
                labelItemList.clear();
            }
            labelItemList.push_back(labelId2Count);
        }
        lastDocId = docId;

        p++;
        if (p % 1000000 == 0)
        {
            std::cout << "[LabelManager] Processing " << p << " items" << std::endl;
        }
    }
    if (labelItemList.size() > 0)
    {
//         std::cout<<"XXX "<<lastDocId<<" "<<labelItemList.size()<<std::endl;
        insertToDocTable_( lastDocId, labelItemList);
        ++docCount;
        labelCount += labelItemList.size();
        labelItemList.clear();

    }
    reader.close();
    docLabelInfo_->flush();
    ldWriter_->flush();
    FSUtil::del(doc2LabelWriter.getPath());
//     MUtil::MEMINFO("After insert to forward indexer");
    idManager_->Flush();
//     MUtil::MEMINFO("After label id manager flush");
    std::cout<<"Average label count : "<<(double)labelCount/ docCount <<std::endl;
    dfInfo_->flush();
    typeInfo_->flush();
//     MUtil::MEMINFO("After label db flush");
    labelStream_.flush();
    if (!updateLabelDb_())
    {
        std::cout << "Update label DB failed" << std::endl;
    }
}

bool LabelManager::updateLabelDb_()
{
    if (collection_name_ == "")
        return true;

    // TODO: keep consistency between collection data and db table when distributed
    PropertyLabel::del_record("collection = '" + collection_name_ + "'");
    uint32_t max_labelid = getMaxLabelID();
    for (uint32_t labelid = 1; labelid <= max_labelid; labelid++)
    {
        izenelib::util::UString labelstr;
        if (!getLabelStringByLabelId(labelid, labelstr)) continue;
        std::string str;
        labelstr.convertString(str, izenelib::util::UString::UTF_8);
        uint32_t df = 0;
        getLabelDF(labelid, df);
        PropertyLabel plabel;
        plabel.setCollection(collection_name_);
        plabel.setLabelName(str);
        plabel.setHitDocsNum(df);
        plabel.save();
    }
    return true;
}

bool LabelManager::getLabelStringByLabelId(labelid_t labelId, izenelib::util::UString& labelStr)
{
    if ( !isOpen() ) return false;

//     bool ret = idManager_->getCachedConceptStringByConceptId(labelId, labelStr);
//     if(ret)
//     {
//         cacheHit_++;
//         return true;
//     }
//     cacheNoHit_++;

    return idManager_->GetStringById(labelId, labelStr);
}

bool LabelManager::getLabelStringListByLabelIdList(const std::vector<uint32_t>& labelIdList, std::vector<izenelib::util::UString>& labelStrList)
{
    if ( !isOpen() ) return false;
    labelStrList.resize(labelIdList.size());
    uint32_t failCount = 0;
    for (std::size_t i=0;i<labelStrList.size();i++)
    {
        bool b = getLabelStringByLabelId(labelIdList[i], labelStrList[i]);
        if (!b) failCount++;
    }
    if ( failCount == labelStrList.size() )
    {
        return false;
    }
    return true;
}

bool LabelManager::checkLabelIdByLabelString(const izenelib::util::UString& labelStr, uint32_t& labelId)
{
    if ( !isOpen() ) return false;
    return idManager_->GetIdByString(labelStr, labelId);
}

bool LabelManager::ExistsLabel(const izenelib::util::UString& text)
{
    hash_t key = idManager_->Hash(text);
    uint32_t label_id = 0;
    if ( labelMap_->get(key, label_id) )
    {
        return true;
    }
    return false;
}

LabelManager::LabelDistributeSSFType::ReaderType* LabelManager::getLabelDistributeReader()
{
    LabelDistributeSSFType::ReaderType* reader = new LabelDistributeSSFType::ReaderType(ldWriter_->getPath());
    reader->open();
    return reader;
}

void LabelManager::encode_(const std::vector<labelid_t>& labels, char** data, uint16_t& len)
{
    if (labels.size()==0)
    {
        *data = new char[1];
        len=0;
        return;
    }

    uint32_t labelCount = labels.size();
    uint16_t max = 65535;
    if (labelCount >= max/sizeof(uint32_t))
    {
        labelCount = max/sizeof(uint32_t);
    }
    izenelib::am::DynArray<uint32_t> ar;
    for (uint32_t i=0;i<labelCount;i++)
        ar.push_back(labels[i]);
    ar.sort();
    uint32_t last = ar.at(0);
    //std::cout<<"[ID]: "<<ar.at(0)<<", ";
    for (uint32_t i=1;i<ar.length();i++)
    {
        //std::cout<<ar.at(i)<<", ";
        ar[i] = ar.at(i)-last;
        last += ar.at(i);
    }
    //std::cout<<std::endl;

    *data = new char[2*labelCount*sizeof(uint32_t)];
    len = 0;

    int32_t ui = 0;
    for (uint32_t i=0;i<ar.length();i++)
    {
        ui = ar.at(i);
        while ((ui & ~0x7F) != 0)
        {
            *(*data + len) = ((uint8_t)((ui & 0x7f) | 0x80));
            ui >>= 7;
            ++len;
        }
        *(*data + len) = (uint8_t)ui;
        ++len;
    }
}

void LabelManager::decode_(char* const data, uint16_t len, std::vector<labelid_t>& labels)
{
    if (len ==0)
        return;

    izenelib::am::DynArray<uint32_t> ar;
    for (uint16_t p = 0; p<len;)
    {
        uint8_t b = *(data+p);
        ++p;

        int32_t i = b & 0x7F;
        for (int32_t shift = 7; (b & 0x80) != 0; shift += 7)
        {
            b = *(data+p);
            ++p;
            i |= (b & 0x7FL) << shift;
        }

        ar.push_back(i);
    }

    labels.push_back(ar.at(0));

    for (uint32_t i =1; i<ar.length(); ++i)
    {
        labelid_t labelId = ar.at(i)+labels[i-1];
        labels.push_back(labelId);
    }
}

bool LabelManager::getLabelDF(labelid_t labelId, uint32_t& df)
{
    if ( !isOpen() || labelId==0 ) return false;
    return dfInfo_->get(labelId, df);
}

void LabelManager::setLabelDF(labelid_t labelId, uint32_t df)
{
    if ( !isOpen() || labelId==0 ) return ;
    dfInfo_->update(labelId, df);
}

void LabelManager::cleanLabelStream()
{
    if ( !isOpen() ) return;
    labelStream_.close();
    std::string labelStreamFile = path_ + "/label.stream";
    labelStream_.open(labelStreamFile.c_str(), std::ios_base::out|std::ios_base::trunc);
    if (!labelStream_.is_open())
    {
        throw FileOperationException(labelStreamFile+" reopen", "LabelManager");
    }
}

double LabelManager::getCacheHitRatio()
{
    double ratio = (double)cacheHit_ / (cacheHit_+cacheNoHit_);
    std::cout<<"Cache Hit Ratio : "<<cacheHit_<<","<<cacheNoHit_<<","<<ratio<<std::endl;
    cacheHit_ = 0;
    cacheNoHit_ = 0;
    return ratio;
}

uint32_t LabelManager::getLabelId_(const izenelib::util::UString& text, uint32_t df, uint8_t type, uint8_t score)
{
    hash_t key = idManager_->Hash(text);
    uint32_t label_id = 0;
    uint32_t odf = 0;
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    if (labelMap_->get(key, label_id))
    {
        getLabelDF(label_id, odf);
    }
    else
    {
        label_id = labelMap_->num_items() + 1;
        idManager_->Put(label_id, text );
        labelMap_->insert(key, label_id);
        labelStream_ << str << "," << df << "," << (uint32_t)score << "," << label_id << endl;
        typeInfo_->update(label_id, type);//update the type in first time.
    }

//  if (!collection_name_.empty())
//  {
//      PropertyLabel plabel;
//      plabel.setCollection(collection_name_);
//      plabel.setLabelName(str);
//      plabel.setHitDocsNum(df);
//      plabel.save();
//  }
    odf += df;
    setLabelDF(label_id, odf);
    return label_id;
}

void LabelManager::insertToDocTable_( uint32_t docId, std::vector<id2count_t>& labelItemList)
{
    //output for topic model
//   izenelib::util::UString ustr;
//   std::string str;
//   std::cout<<"KKPPEE;";
//   for(uint32_t i=0;i<labelItemList.size();i++)
//   {
//     getLabelStringByLabelId(labelItemList[i].first, ustr);
//     ustr.convertString(str, izenelib::util::UString::UTF_8);
//     boost::replace_all(str, " ", "-");
//     for(uint32_t j=0;j<labelItemList[i].second; j++)
//     {
//       std::cout<<str;
//       if( i!=labelItemList.size()-1 || j!=labelItemList[i].second-1 )
//       {
//         std::cout<<" ";
//       }
//     }
//   }
//   std::cout<<std::endl;

    if(no_doc_container_) return;
    std::vector<uint32_t> labelIdList;
    std::vector<uint32_t> sorted_label;
    selectDocLabels(labelItemList, labelIdList, sorted_label);

    char* ptr = NULL;
    uint16_t vsize;
    encode_(labelIdList, &ptr, vsize);
    docTableMutex_.lock();
    docLabelInfo_->insert_doc(docId, ptr, vsize);
    docTableMutex_.unlock();
    delete[] ptr;
    sorted_label_->update(docId, sorted_label);
    ldWriter_->append(docId, labelItemList);
}

bool  LabelManager::selectDocLabels(
    std::vector<id2count_t>& labelItemList,
    std::vector<uint32_t>& labelIdList,
    std::vector<uint32_t>& sorted_label)
{
    typedef izenelib::util::second_greater<id2count_t> greater_than;
    const uint32_t MAX_LABEL_NUM_PER_DOC=100;
    const uint32_t MAX_SORTED = 30;
    uint32_t df=0;
    float smoothFactor=(float)info_/10000;
    if (smoothFactor<8)
        smoothFactor=8;
    for (uint32_t i=0;i<labelItemList.size();i++)
    {
        getLabelDF(labelItemList[i].first,df);
        double score = sqrt(labelItemList[i].second+1.0)*log((double)info_/(df+smoothFactor)+2);
        labelItemList[i].second=(uint32_t)score;
    }
    std::sort(labelItemList.begin(), labelItemList.end(), greater_than());
    uint32_t count_num = std::min(MAX_LABEL_NUM_PER_DOC, (uint32_t)labelItemList.size());
    uint32_t sorted_num = std::min(MAX_SORTED, (uint32_t)labelItemList.size());
    labelIdList.resize(count_num);
    for (uint32_t i=0;i<count_num;i++)
    {
        labelIdList[i]=labelItemList[i].first;
    }
    std::sort(labelIdList.begin(), labelIdList.end() );
    sorted_label.resize(sorted_num);
    for (uint32_t i=0;i<sorted_num;i++)
    {
        sorted_label[i]=labelItemList[i].first;
    }
    return true;
}
