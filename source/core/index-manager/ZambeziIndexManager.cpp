#include "ZambeziIndexManager.h"
#include <la-manager/AttrTokenizeWrapper.h>
#include <configuration-manager/ZambeziConfig.h>
#include <document-manager/Document.h>
#include <glog/logging.h>
#include <fstream>

using namespace sf1r;

ZambeziIndexManager::ZambeziIndexManager(
    const ZambeziConfig& config,
    izenelib::ir::Zambezi::AttrScoreInvertedIndex& indexer)
    : config_(config)
    , indexer_(indexer)
{
}

void ZambeziIndexManager::postBuildFromSCD(time_t timestamp)
{
    indexer_.flush();

    std::ofstream ofs(config_.indexFilePath.c_str(), std::ios_base::binary);
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << config_.indexFilePath;
        return;
    }

    try
    {
        indexer_.save(ofs);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in writing file: " << e.what()
                   << ", path: " << config_.indexFilePath;
    }
}

bool ZambeziIndexManager::insertDocument(
    const Document& doc,
    time_t timestamp)
{
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

bool ZambeziIndexManager::buildDocument_(const Document& doc)
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
    std::vector<std::string> tokenList;
    std::vector<uint32_t> scoreList;

    for (std::vector<std::pair<std::string, double> >::const_iterator it =
             tokenScoreList.begin(); it != tokenScoreList.end(); ++it)
    {
        tokenList.push_back(it->first);
        scoreList.push_back(uint32_t(it->second));
    }

    docid_t docId = doc.getId();
    indexer_.insertDoc(docId, tokenList, scoreList);

    return true;
}
