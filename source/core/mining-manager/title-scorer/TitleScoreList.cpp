#include "TitleScoreList.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace sf1r
{

TitleScoreList::TitleScoreList(
        const std::string& dirPath,
        const std::string& propName,
        bool isDebug)
    : dirPath_(dirPath)
    , propName_(propName)
    , lastDocScoreDocid_(0)
    , isDebug_(isDebug)
{
}

bool TitleScoreList::open()
{
    return loadDocumentScore();   
}

bool TitleScoreList::saveDocumentScore(unsigned int last_doc)
{
    std::string documentScorePath = dirPath_ + "/doc.Score";
    std::string documentNumPath = dirPath_ + "/doc.size";
    std::string documenttxt = dirPath_ + "/doc.txt";
    fstream fout;
    fstream fout_size;
    fstream fout_txt;
    fout.open(documentScorePath.c_str(), ios::app | ios::out | ios::binary);
    fout_size.open(documentNumPath.c_str(), ios::out | ios::binary);
    fout_txt.open(documenttxt.c_str(), ios::out);

    if(fout_size.is_open())
    {
        fout_size.write(reinterpret_cast< char *>(&last_doc), sizeof(unsigned int));
        fout_size.close();
    }

    if(fout.is_open())
    {
        for (unsigned int i = lastDocScoreDocid_ + 1; i <= last_doc; ++i)
            fout.write(reinterpret_cast< char *>(&document_Scores_[i]), sizeof(double));
        fout.close();
        lastDocScoreDocid_ = last_doc;
    }

    if(fout_txt.is_open())
    {
        for (int i = 1; i < last_doc + 1; ++i)
        {
            fout_txt << "docid:" << i << " score:" << document_Scores_[i] << endl;
        }
        fout_txt.close();
    }

    return true;
}

bool TitleScoreList::loadDocumentScore()
{
    std::string documentScorePath = dirPath_ + "/doc.Score";
    std::string documentNumPath = dirPath_ + "/doc.size";
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

    if (document_Scores_.size() != lastDocScoreDocid_ + 1)
        return false;

    return true;
}

bool TitleScoreList::clearDocumentScore()
{
    std::string documentScorePath = dirPath_ + "/doc.Score";
    std::string documentNumPath = dirPath_ + "/doc.size";
    document_Scores_.clear();
    try
    {
        if (boost::filesystem::exists(documentScorePath))
            boost::filesystem::remove_all(documentScorePath);
        
        if (boost::filesystem::exists(documentNumPath))
            boost::filesystem::remove_all(documentNumPath);   
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
    }
}

void TitleScoreList::resize(unsigned int size)
{
    document_Scores_.resize(size);
}

bool TitleScoreList::insert(unsigned int index, double value)
{
    if (index < document_Scores_.size())
    {
        document_Scores_[index] = value;    
    }
    else
        return false;

    return true;
}

double TitleScoreList::getScore(unsigned int index)
{
    if (index < document_Scores_.size())
        return document_Scores_[index];
    return -1;
}

docid_t TitleScoreList::getLastDocId_()
{
    return lastDocScoreDocid_;
}

}