#include "ZambeziIndexManager.h"
#include "./zambezi-tokenizer/ZambeziTokenizer.h"
#include <la-manager/AttrTokenizeWrapper.h>
#include <configuration-manager/ZambeziConfig.h>
#include <document-manager/Document.h>
#include <glog/logging.h>
#include <fstream>

#include <document-manager/DocumentManager.h>
using namespace sf1r;

ZambeziIndexManager::ZambeziIndexManager(
    const ZambeziConfig& config,
    const std::vector<std::string>& properties,
    std::map<std::string, ZambeziBaseIndex*>& property_index_map,
    ZambeziTokenizer* zambeziTokenizer,
    boost::shared_ptr<DocumentManager> documentManager)
    : indexDocCount_(0)
    , config_(config)
    , properties_(properties)
    , zambeziTokenizer_(zambeziTokenizer)
    , property_index_map_(property_index_map)
    , documentManager_(documentManager)
{
}

ZambeziIndexManager::~ZambeziIndexManager()
{
    LOG(INFO) << " ~ZambeziIndexManager() ";
    //postProcessForAPI();
}

// build scd and creatdocument comes sequentially
// it is controlled in zookeeper; all the index data comes in single thread in distribute env; 
// and index_binlog for realtime index is not needed. 
// If the index is down, the index rebuild from last backup;

// WARNING: if the index is not used in distribute env;

void ZambeziIndexManager::postProcessForAPI()
{
    postBuildFromSCD(1);
    indexDocCount_ = 0;
}

void ZambeziIndexManager::postBuildFromSCD(time_t timestamp)
{
    for (std::map<std::string, ZambeziBaseIndex*>::iterator i = property_index_map_.begin(); i != property_index_map_.end(); ++i)
    {
        i->second->flush();
        
        std::string indexPath = config_.indexFilePath + "_" + i->first;
        std::ofstream ofs(indexPath.c_str(), std::ios_base::binary);
        if (!ofs)
        {
            LOG(ERROR) << "failed opening file " << indexPath;
            return;
        }

        try
        {
            i->second->save(ofs);
        }
        catch (const std::exception& e)
        {
            LOG(ERROR) << "exception in writing file: " << e.what()
                       << ", path: " << indexPath;
        }
    }
}

bool ZambeziIndexManager::insertDocument(
    const Document& doc,
    time_t timestamp)
{
    //never flush during the create document ...
    /*if (isRealTime)
        indexDocCount_++;
    
    if (indexDocCount_ == MAX_API_INDEXDOC)
        postProcessForAPI();*/

    return buildDocument_(doc);
}

bool ZambeziIndexManager::updateDocument(
    const Document& olddoc,
    const Document& old_rtype_doc,
    const Document& newdoc,
    int updateType,
    time_t timestamp)
{
    return buildDocument_(newdoc);
}

bool ZambeziIndexManager::insertDocIndex_(
    const docid_t docId, 
    const std::string property,
    const std::vector<std::pair<std::string, int> >& tokenScoreList)
{       
    std::vector<std::string> tokenList;
    std::vector<uint32_t> scoreList;
    for (std::vector<std::pair<std::string, int> >::const_iterator it =
             tokenScoreList.begin(); it != tokenScoreList.end(); ++it)
    {
        tokenList.push_back(it->first);
        scoreList.push_back(uint32_t(it->second));
    }
    property_index_map_[property]->insertDoc(docId, tokenList, scoreList);

    return true; 
}

bool ZambeziIndexManager::buildDocument_Normal_(const Document& doc, const std::string& property)
{
    std::string proValue;
    doc.getProperty(property, proValue);

    std::vector<std::pair<std::string, int> > tokenScoreList;
    if (!proValue.empty())
    {
        zambeziTokenizer_->GetTokenResults(proValue, tokenScoreList);
        docid_t docId = doc.getId();
        insertDocIndex_(docId, property, tokenScoreList);
    }

    return true;
}

bool ZambeziIndexManager::buildDocument_Combined_(const Document& doc, const std::string& property)
{   
    std::vector<string> subProperties;
    for (unsigned int i = 0; i < config_.virtualPropeties.size(); ++i)
    {
        if (config_.virtualPropeties[i].name == property)
        {
            subProperties = config_.virtualPropeties[i].subProperties;
            break;
        }
    }

    std::string proValue;
    std::string combined_proValue;
    for (unsigned int i = 0; i < subProperties.size(); ++i)
    {
        proValue.clear();
        doc.getProperty(subProperties[i], proValue);
        combined_proValue += proValue;
    }

    if (!combined_proValue.empty())
    {
        std::vector<std::pair<std::string, int> > tokenScoreList;
        zambeziTokenizer_->GetTokenResults(combined_proValue, tokenScoreList);
        docid_t docId = doc.getId();
        insertDocIndex_(docId, property, tokenScoreList);        
    }

    return true;
}

bool ZambeziIndexManager::buildDocument_Attr_(const Document& doc, const std::string& property)
{
    std::vector<std::string> propNameList;
    std::vector<std::string> propValueList;
    propNameList.push_back("Title");
    propNameList.push_back("Attribute");
    propNameList.push_back("Category");
    propNameList.push_back("OriginalCategory");
    propNameList.push_back("Source");

    for (std::vector<std::string>::iterator i = propNameList.begin();
         i != propNameList.end(); ++i)
    {
        std::string propValue;
        doc.getProperty(*i, propValue);
        if (*i == "Attribute")
        {
            std::string brand;
            doc.getProperty("Brand", brand);
            if (!brand.empty())
            {
                propValue += (",brand:" + brand);
            }
        }
        propValueList.push_back(propValue);
    }

    std::vector<std::pair<std::string, double> > tokenScoreList;
    AttrTokenizeWrapper::get()->attr_tokenize_index(propValueList[0],
                                                    propValueList[1],
                                                    propValueList[2],
                                                    propValueList[3],
                                                    propValueList[4],
                                                    tokenScoreList);

    docid_t docId = doc.getId();
    std::vector<std::pair<std::string, int> > tokenScoreList1(tokenScoreList.begin(), tokenScoreList.end());

    insertDocIndex_(docId, property, tokenScoreList1);

    return true;
}

bool ZambeziIndexManager::buildDocument_(const Document& doc)
{
    for (std::vector<std::string>::const_iterator i = properties_.begin(); i != properties_.end(); ++i)
    {
        std::map<std::string, PropertyStatus>::const_iterator iter
         =  config_.property_status_map.find(*i);

        if (iter == config_.property_status_map.end())
            continue;

        if (iter->second.isCombined && iter->second.isAttr)
        {
            buildDocument_Attr_(doc, *i);
        }
        else if (iter->second.isCombined)
        {
            buildDocument_Combined_(doc, *i);
        }
        else
        {
            buildDocument_Normal_(doc, *i);
        }
    }
    return true;
}
