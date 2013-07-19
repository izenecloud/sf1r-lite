#include "TitleScoreMiningTask.h"
#include "ProductTokenizer.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/KNlpWrapper.h>
#include <document-manager/DocumentManager.h>

namespace sf1r
{

TitleScoreMiningTask::TitleScoreMiningTask(boost::shared_ptr<DocumentManager>& document_manager
                    , std::string data_root_path
                    , ProductTokenizer* tokenizer)
    : couldUse_(false)
    , document_manager_(document_manager)
    , data_root_path_(data_root_path)
    , lastDocScoreDocid_(0)
    , tokenizer_(tokenizer)
{
    loadDocumentScore();
}

TitleScoreMiningTask::~TitleScoreMiningTask()
{

}


bool TitleScoreMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    bool failed = (doc.getId() == 0);
    if (!failed)
    {
        const std::string pname = "Title";
        std::string pattern;
        doc.getString(pname, pattern);
        double scoreSum = 0;
        tokenizer_->GetQuerySumScore(pattern, scoreSum);
        document_Scores_.push_back(scoreSum);
    }
    else
        document_Scores_.push_back(0);

    return true;
}

docid_t TitleScoreMiningTask::getLastDocId()
{

    return lastDocScoreDocid_ + 1;
}

bool TitleScoreMiningTask::preProcess()
{
    if (lastDocScoreDocid_ == 0)
    {
        clearDocumentScore();
    }
    return true;
}

bool TitleScoreMiningTask::postProcess()
{
    LOG (INFO) << "Save Document Scores ......" ;
    saveDocumentScore();
    return true;
}

void TitleScoreMiningTask::saveDocumentScore()
{
    std::string documentScorePath = data_root_path_ + "/doc.Score";
    std::string documentNumPath = data_root_path_ + "/doc.size";
    fstream fout;
    fstream fout_size;
    fout.open(documentScorePath.c_str(), ios::app | ios::out | ios::binary);
    fout_size.open(documentNumPath.c_str(), ios::out | ios::binary);
    unsigned int max_doc = document_manager_->getMaxDocId();

    if(fout_size.is_open())
    {
        fout_size.write(reinterpret_cast< char *>(&max_doc), sizeof(unsigned int));
        fout_size.close();
    }

    if(fout.is_open())
    {
        for (unsigned int i = lastDocScoreDocid_ + 1; i <= max_doc; ++i)
            fout.write(reinterpret_cast< char *>(&document_Scores_[i]), sizeof(double));
        fout.close();
    }
}

void TitleScoreMiningTask::loadDocumentScore()
{
    std::string documentScorePath = data_root_path_ + "/doc.Score";
    std::string documentNumPath = data_root_path_ + "/doc.size";
    fstream fin;
    fstream fin_size;
    fin.open(documentScorePath.c_str(), ios::binary | ios::in);
    fin_size.open(documentNumPath.c_str(), ios::binary | ios::in);

    if (fin_size.is_open())
    {
        fin_size.read(reinterpret_cast< char *>(&lastDocScoreDocid_),  sizeof(docid_t));
    }

    document_Scores_.reserve(lastDocScoreDocid_ + 1);
    document_Scores_.push_back(0);
    if (fin.is_open())
    {
        while(!fin.eof())
        {
            double score = 0;
            if (fin.read(reinterpret_cast< char *>(&score),  sizeof(double)) != 0)
            {
                document_Scores_.push_back(score);
            }
        }
        fin.close();
    }
    else
        LOG (WARNING) << "open DocumentScore file failed";

    if (document_Scores_.size() == lastDocScoreDocid_ + 1)
    {
        couldUse_ = true;
        LOG (INFO) << "DocumentScore can be used now";
    }
}

void TitleScoreMiningTask::clearDocumentScore()
{
    std::string documentScorePath = data_root_path_ + "/doc.Score";
    document_Scores_.clear();
    document_Scores_.reserve(document_manager_->getMaxDocId() + 1);
    document_Scores_.push_back(0);
    try
    {
        if (boost::filesystem::exists(documentScorePath))
            boost::filesystem::remove_all(documentScorePath);
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
    }
}

const std::vector<double>& TitleScoreMiningTask::getDocumentScore()
{
    return document_Scores_;
}

bool TitleScoreMiningTask::canUse()
{
    return couldUse_;
}
}