# include "IncrementalFuzzyIndex.h"
#include <common/JobScheduler.h>
#include <common/Utilities.h>

#include <la-manager/LAManager.h>//
#include <document-manager/Document.h>

#include <document-manager/DocumentManager.h>
#include <ir/index_manager/index/LAInput.h>
#include <icma/icma.h>
#include <fstream>

using namespace cma;
namespace sf1r
{
    ForwardIndex::ForwardIndex(std::string index_path, unsigned int Max_Doc_Num)
    {
        index_path_ = index_path + ".fd.idx";
        Max_Doc_Num_ = Max_Doc_Num;
        IndexCount_ = 0;
        IndexItems_.clear();
        BitMapMatrix_ = new BitMap[Max_Doc_Num_];
        start_docid_ = 0;
        max_docid_ = 0;
    }

    ForwardIndex::~ForwardIndex()
    {
        if (BitMapMatrix_ != NULL)
        {
            delete [] BitMapMatrix_;
        }
        BitMapMatrix_ = NULL;
    }

    bool ForwardIndex::checkLocalIndex_()
    {
        if (bfs::exists(index_path_))
        {
            if (load_())
            {
                return true;
            }
            else
            {
                bfs::remove(index_path_);
                return false;
            }
        }
        else
            return false;
    }

    uint32_t ForwardIndex::getIndexCount_()
    {
        return IndexCount_;
    }

    void ForwardIndex::addDocForwardIndex_(uint32_t docId
                                        , std::vector<uint32_t>& termidList
                                        , std::vector<unsigned short>& posList)
    {
        if (start_docid_ == 0)
        {
            start_docid_ = docId;
        }

        BitMapMatrix_[docId - start_docid_].offset_ = IndexCount_;
        unsigned int count = termidList.size();
        for (unsigned int i = 0; i < count; ++i)
        {
            addOneIndexIterms_(docId, termidList[i], posList[i]);
            IndexCount_++;
        }
        max_docid_ = docId;
    }

    void ForwardIndex::addOneIndexIterms_(uint32_t docId, uint32_t termid, unsigned short pos)
    {
        IndexItem indexItem;
        indexItem.setIndexItem_(docId, termid, pos);
        IndexItems_.push_back(indexItem);
    }

    uint32_t ForwardIndex::getScore(std::vector<uint32_t> &v, uint32_t & docid)
    {
        uint32_t start = BitMapMatrix_[docid - start_docid_].offset_;
        uint32_t end = BitMapMatrix_[docid - start_docid_ + 1].offset_;
        std::vector<uint32_t> doclist;
        for (uint32_t i = start; i < end; ++i)
        {
            doclist.push_back(IndexItems_[i].Termid_);
        }
        return includeNum(v, doclist);
    }

    uint32_t ForwardIndex::includeNum(const vector<uint32_t>& vec1,const vector<uint32_t>& vec2)
    {
        uint32_t k = 0;
        uint32_t j = 0;
        for (uint32_t i = 0; i < vec2.size(); ++i)
        {
            if (j < vec1.size())
            {
                while (vec1[j] == vec2[i])
                {
                    if (j == vec1.size() - 1)
                    {
                        k++;
                        j = 0;
                        break;
                    }
                    j++;
                    i++;
                }
            }
            else
                continue;
        }
        return k;
    }

    void ForwardIndex::setPosition_(std::vector<pair<uint32_t, unsigned short> >*& v)
    {
        for (std::vector<pair<uint32_t, unsigned short> >::const_iterator it = v->begin();
                it != v->end(); ++it)
        {
            BitMapMatrix_[it->first - start_docid_].setBitMap(it->second);//ADD OFFSET.....
        }
    }

    bool ForwardIndex::getTermIDByPos_(const uint32_t& docid
                                    , const std::vector<unsigned short>& pos
                                    , std::vector<uint32_t>& termidList)
    {
        std::set<uint32_t> termidSet;
        std::vector<IndexItem>::const_iterator it = IndexItems_.begin();
        std::vector<IndexItem>::const_iterator its = IndexItems_.begin();
        for (; it != IndexItems_.end(); ++its,++it)
        {
            if (it->Docid_ == docid)
                break;
        }
        for (std::vector<unsigned short>::const_iterator i = pos.begin(); i != pos.end(); ++i)
        {
            uint32_t k = 0;
            it = its;
            while (k < MAX_POS_LEN)
            {
                if (it->Docid_ == docid && it->Pos_ == *i)
                {
                    termidSet.insert(it->Termid_);
                    break;
                }
                ++k;
                ++it;
            }
        }
        for (std::set<uint32_t>::iterator i = termidSet.begin(); i != termidSet.end(); ++i)
        {
            termidList.push_back(*i);
        }
        return true;
    }

    void ForwardIndex::resetBitMapMatrix()
    {
        for (unsigned int i = 0; i < (max_docid_ - start_docid_) ; ++i)//for (int i = 0; i < (max_docid_ - start_docid_); i++ )....
        {
            BitMapMatrix_[i].reset();
        }
    }

    void ForwardIndex::getLongestSubString_(std::vector<uint32_t>& termidList, const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList, uint32_t& hitdoc)
    {
        std::vector<pair<uint32_t, unsigned short> >::const_iterator it;
        BitMap b;
        b.reset();
        vector<unsigned short> temp;
        vector<unsigned short> longest;
        uint32_t resultDocid = 0;
        for (std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator iter = resultList.begin(); iter != resultList.end(); ++iter)
        {
            if (longest.size() > MAX_SUB_LEN)
            {
                break;
            }
            for (it = (**iter).begin(); it != (**iter).end(); ++it)
            {
                temp.clear();
                bool isNext = true;
                for (unsigned short i = 0; i < MAX_POS_LEN; ++i)
                {
                    if (BitMapMatrix_[it->first - start_docid_].getBitMap(i))
                    {
                        temp.push_back(i);
                        isNext = true;
                    }
                    else
                    {
                        if (isNext == false)
                        {
                            if (temp.size() > longest.size())
                            {
                                longest = temp;
                                resultDocid = it->first;
                            }
                            temp.clear();
                        }
                        else
                        {
                            isNext = false;
                        }
                    }
                }
                if (temp.size() > longest.size())
                {
                    longest = temp;
                    resultDocid = it->first;
                    temp.clear();
                }
                if (longest.size() > MAX_SUB_LEN)
                {
                    break;
                }
            }
        }
        std::vector<unsigned short> hitPos;
        LOG(INFO)<<"[INFO] Resulst Docid:"<<resultDocid<<" And hit pos:"<<endl;
        hitdoc = resultDocid;
        for (unsigned int i = 0; (i < longest.size() && i < MAX_SUB_LEN); ++i)
        {
            cout<<longest[i]<<" ";
            hitPos.push_back(longest[i]);
        }
        cout<<endl;
        /**
        If longest.size() is less than 3, means this is almost no doc similar with this doc;
        In order to prevent get those not releated files;
        */
        if (longest.size() < 3)
        {
            return;
        }
        getTermIDByPos_(resultDocid, hitPos, termidList);
    }

    void ForwardIndex::print()
    {
        cout<<"IndexCount_:"<<IndexCount_<<endl;
    }

    bool ForwardIndex::save_(std::string path)
    {
        string index_path = index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "wb")) == NULL)
        {
            LOG(INFO) <<"Cannot open output file"<<endl;
            return false;
        }

        unsigned int totalSize = IndexCount_ * sizeof(IndexItem);
        fwrite(&totalSize, sizeof(totalSize), 1, file);

        fwrite(&start_docid_, sizeof(unsigned int), 1, file);
        fwrite(&max_docid_, sizeof(unsigned int), 1, file);
        for (uint32_t i = start_docid_; i <= max_docid_; ++i)
        {
            fwrite(&(BitMapMatrix_[i - start_docid_].offset_), sizeof(unsigned int), 1, file);
        }
        std::vector<IndexItem>::const_iterator it = IndexItems_.begin();
        for (; it != IndexItems_.end(); ++it)
        {
            fwrite(&(*it), sizeof(IndexItem), 1, file);
        }
        fclose(file);
        return true;
    }

    bool ForwardIndex::load_(std::string path)
    {
        string index_path = index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "rb")) == NULL)
        {
            LOG(INFO) <<"Cannot open output file"<<endl;
            return false;
        }
        unsigned int totalSize;
        unsigned int readSize = 0;

        readSize = fread(&totalSize, sizeof(totalSize), 1, file);
        if (readSize != 1)
        {
            LOG(INFO) <<"Fread Wrong!!"<<endl;
            return false;
        }

        readSize = fread(&start_docid_, sizeof(unsigned int), 1, file);
        readSize = fread(&max_docid_, sizeof(unsigned int), 1, file);
        for (uint32_t i = start_docid_; i <= max_docid_; ++i)
        {
            readSize = fread(&(BitMapMatrix_[i - start_docid_].offset_), sizeof(unsigned int), 1, file);
        }
        IndexCount_ = totalSize/(sizeof(IndexItem));
        for (unsigned int i = 0; i < IndexCount_; ++i)
        {
            IndexItem indexItem;
            readSize = fread(&indexItem, sizeof(IndexItem), 1, file);

            IndexItems_.push_back(indexItem);
        }
        fclose(file);
        return true;
    }

    void ForwardIndex::addIndexItem_(const IndexItem& indexItem)
    {
        IndexItems_.push_back(indexItem);
        if (start_docid_ == 0)
        {
            start_docid_ = indexItem.Docid_;
        }
        if (indexItem.Docid_ > max_docid_)
        {
            max_docid_ = indexItem.Docid_;
        }
        IndexCount_++;
    }

    std::vector<IndexItem>& ForwardIndex::getIndexItem_()
    {
        return IndexItems_;
    }

    std::string ForwardIndex::getPath()
    {
        return index_path_;
    }

/**
*/
    InvertedIndex::InvertedIndex(std::string file_path,
                   unsigned int Max_Doc_Num,
                   cma::Analyzer* &analyzer)
        : analyzer_(analyzer)
    {
        Increment_index_path_ = file_path + ".inc.idx";
        Max_Doc_Num_ = Max_Doc_Num;
        termidNum_ = 0;
        allIndexDocNum_ = 0;
    }

    InvertedIndex::~InvertedIndex()
    {
    }

    bool InvertedIndex::checkLocalIndex_()
    {
        if (bfs::exists(Increment_index_path_))
        {
            if (load_())
            {
                return true;
            }
            else
            {
                bfs::remove(Increment_index_path_);
                return false;
            }
        }
        else
            return false;
    }

    void InvertedIndex::addIndex_(uint32_t docid, std::vector<uint32_t> termidList, std::vector<unsigned short>& posList)
    {
        unsigned int count = termidList.size();
        allIndexDocNum_ += count;
        for (unsigned int j = 0; j < count; ++j)
        {
            pair<uint32_t, uint32_t> temp(termidList[j], 0);
            std::set<std::pair<uint32_t, uint32_t>, pairLess >::iterator i = termidList_.find(temp);;
            if (i != termidList_.end())
            {
                uint32_t offset = (*i).second;
                pair<uint32_t, unsigned short> term(docid, posList[j]);
                docidLists_[offset].push_back(term);//xxx
            }
            else
            {
                std::vector<pair<uint32_t, unsigned short> > tmpDocList;
                pair<uint32_t, unsigned short> term(docid, posList[j]);
                tmpDocList.push_back(term);
                pair<uint32_t, uint32_t> termid(termidList[j], docidLists_.size());
                termidList_.insert(termid);
                docidLists_.push_back(tmpDocList);
                termidNum_++;
            }
        }
    }

    void InvertedIndex::getResultAND_(const std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList, std::vector<double>& ResultListSimilarity)
    {
        std::vector<std::vector<pair<uint32_t, unsigned short> >* > docidLists;
        std::vector<uint32_t> sizeLists;
        uint32_t size;
        std::vector<pair<uint32_t, unsigned short> >* vresult = NULL;
        for (std::vector<uint32_t>::const_iterator i = termidList.begin(); i != termidList.end(); ++i)
        {
            if (!getTermIdResult_(*i, vresult, size))
                return;
            docidLists.push_back(vresult);
            sizeLists.push_back(size);
        }

        mergeAnd_(docidLists, resultList, ResultListSimilarity);
    }

    bool InvertedIndex::getTermIdResult_(const uint32_t& termid, std::vector<pair<uint32_t, unsigned short> >*& v, uint32_t& size)
    {
        pair<uint32_t, uint32_t> temp(termid, 0);

        std::set<std::pair<uint32_t, uint32_t> >::iterator i = termidList_.find(temp);
        if (i != termidList_.end())
        {
            v = &(docidLists_[(*i).second]);
            size = docidLists_[(*i).second].size();
            return true;
        }
        else
        {
            return false;
        }
    }

    void InvertedIndex::getTermIdPos_(const uint32_t& termid, std::vector<pair<uint32_t, unsigned short> >*& v)
    {
        pair<uint32_t, uint32_t> temp(termid, 0);
        std::set<std::pair<uint32_t, uint32_t> >::iterator i = termidList_.find(temp);
        if (i != termidList_.end())
        {
            v = &(docidLists_[(*i).second]);
        }
    }

    void InvertedIndex::getResultORFuzzy_(const std::vector<uint32_t>& termidList, std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList)
    {
        std::vector<pair<uint32_t, unsigned short> >* v = NULL;
        uint32_t size;
        for (std::vector<uint32_t>::const_iterator i = termidList.begin(); i != termidList.end(); ++i)
        {
            getTermIdResult_(*i, v, size);
            resultList.push_back(v);
        }
    }

    void InvertedIndex::mergeAnd_(const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& docidLists, std::vector<uint32_t>& resultList, std::vector<double>& ResultListSimilarity)// get add result; all the vector<docid> is sorted;
    {
        izenelib::util::ClockTimer timer;
        unsigned int size = docidLists.size();
        if (size == 0)
        {
            return;
        }
        unsigned int min = 0;
        unsigned int label = 0;
        for (unsigned int i = 0; i < size; ++i)
        {
            if (min == 0)
            {
                min = (*(docidLists[i])).size();
                label = i;
                continue;
            }
            if (min > (*(docidLists[i])).size())
            {
                min = (*(docidLists[i])).size();
                label = i;
            }
        }

        std::vector<pair<uint32_t, unsigned short> >::const_iterator iter = (*(docidLists[label])).begin();
        vector<int> iteratorList;
        for (std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator i = docidLists.begin(); i != docidLists.end(); ++i)
        {
            iteratorList.push_back(0);
        }

        for ( ; iter != (*(docidLists[label])).end(); ++iter)
        {
            std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator i = docidLists.begin();
            uint32_t count = 0;
            bool flag = true;
            while (count < size)
            {
                if (count == label)
                {
                    count++;
                    i++;
                    continue;
                }
                iteratorList[count] = BinSearch_(**i, iteratorList[count], (**i).size(), (*iter).first);//XXXOPTIM
                if ( iteratorList[count] == -1)
                {
                    flag = false;
                    break;
                }
                i++;
                count++;
            }

            if (flag)
            {
                if (resultList.size() == 0)
                {
                    resultList.push_back((*iter).first);
                }
                if (resultList.size() > 0 && (*iter).first != resultList[resultList.size()-1])//may has the same
                {
                    resultList.push_back((*iter).first);
                }
            }
        }

        LOG(INFO)<<"mergeAnd and cost:"<<timer.elapsed()<<" seconds"<<endl;
    }

    void InvertedIndex::print()
    {
        cout<<"docidNumbers_:";
        unsigned int k = 0;
        for (uint32_t i = 0; i < termidList_.size(); ++i)
        {
            k += docidLists_[i].size();
        }
        cout<<k<<endl<<"termidList_.....:"<<termidList_.size()<<endl;
    }

    bool InvertedIndex::save_(std::string path)
    {
        string index_path = Increment_index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "wb")) == NULL)
        {
            LOG(INFO) << "Cannot open output file" << endl;
            return false;
        }
        unsigned int count = termidList_.size();
        fwrite(&count, sizeof(count), 1, file);
        for (std::set<std::pair<uint32_t, uint32_t>, pairLess>::iterator i = termidList_.begin(); i != termidList_.end(); ++i)
        {
            fwrite(&(*i), sizeof(pair<uint32_t, uint32_t>), 1, file);
        }
        fwrite(&allIndexDocNum_, sizeof(allIndexDocNum_), 1, file);
        for (std::vector<std::vector<pair<uint32_t, unsigned short> > >::iterator i = docidLists_.begin(); i != docidLists_.end(); ++i)
        {
            unsigned int size = (*i).size();
            fwrite(&size, sizeof(size), 1, file);
            for (std::vector<pair<uint32_t, unsigned short> >::iterator it = (*i).begin(); it != (*i).end(); ++it)
            {
                fwrite(&(*it), sizeof(pair<uint32_t, unsigned short>), 1, file);
            }
        }
        fclose(file);
        return true;
    }

    bool InvertedIndex::load_(std::string path)
    {
        string index_path = Increment_index_path_ + path;
        FILE *file;
        if ((file = fopen(index_path.c_str(), "rb")) == NULL)
        {
            LOG(INFO) << "Cannot open output file" << endl;
            return false;
        }
        unsigned int count = 0;
        unsigned int readSize = 0;
        readSize = fread(&count, sizeof(unsigned int), 1, file);
        if (readSize != 1)
        {
            LOG(INFO) <<"Fread Wrong!!"<<endl;
            return false;
        }

        for (unsigned int i = 0; i < count; ++i)
        {
            pair<uint32_t, uint32_t> doc;
            readSize = fread(&doc, sizeof(doc), 1, file);
            termidList_.insert(doc);
        }

        readSize = fread(&allIndexDocNum_, sizeof(allIndexDocNum_), 1, file);
        if (readSize != 1)
        {
            LOG(INFO) <<"Fread Wrong!!"<<endl;
            return false;
        }

        unsigned int i = 0;
        while ( i < allIndexDocNum_)
        {
            unsigned int size = 0;
            std::vector<pair<uint32_t, unsigned short> > v;
            readSize = fread(&size, sizeof(count), 1, file);

            for (unsigned int j = 0; j < size; ++j)
            {
                pair<uint32_t, unsigned short> p;
                readSize = fread(&p, sizeof(pair<uint32_t, unsigned short>), 1, file);

                v.push_back(p);
                i++;
            }
            docidLists_.push_back(v);
            v.clear();
        }
        fclose(file);
        return true;
    }

    void InvertedIndex::prepare_index_()
    {
        std::vector<std::vector<pair<uint32_t, unsigned short> > >::iterator iter = docidLists_.begin();
        for (; iter != docidLists_.end(); ++iter)
        {
            sort((*iter).begin(), (*iter).end());
        }
    }
    void InvertedIndex::addTerm_(const pair<uint32_t, uint32_t> &termid, const std::vector<pair<uint32_t, unsigned short> >& docids)
    {
        std::set<std::pair<uint32_t, uint32_t>, pairLess>::const_iterator it = termidList_.find(termid);
        allIndexDocNum_ += docids.size();
        if (it != termidList_.end())
        {
            for (std::vector<pair<uint32_t, unsigned short> >::const_iterator iter = docids.begin(); iter != docids.end(); ++iter)
            {
                docidLists_[it->second].push_back(*iter);
            }
        }
        else
        {
            pair<uint32_t, uint32_t>  p(termid.first, docidLists_.size());
            std::vector<pair<uint32_t, unsigned short> > v;
            docidLists_.push_back(v);
            termidList_.insert(p);
            for (std::vector<pair<uint32_t, unsigned short> >::const_iterator iter = docids.begin(); iter != docids.end(); ++iter)
            {
                docidLists_[docidLists_.size()-1].push_back(*iter);
            }
        }
    }

    std::set<std::pair<uint32_t, uint32_t>, pairLess>& InvertedIndex::gettermidList_()
    {
        return termidList_;
    }

    std::vector<std::vector<pair<uint32_t, unsigned short> > >& InvertedIndex::getdocidLists_()
    {
        return docidLists_;
    }

    const std::string& InvertedIndex::getPath() const
    {
        return Increment_index_path_;
    }

    int InvertedIndex::BinSearch_(std::vector<pair<unsigned int, unsigned short> >&A, int min, int max, unsigned int key)
    {
        while (min <= max)
        {
            int mid = (min + max)/2;
            if (A[mid].first == key)
                return mid;
            else if (A[mid].first > key)
                max = mid - 1;
            else
                min = mid + 1;
        }
        return -1;
    }

    /**
    */
    IncrementalFuzzyIndex::IncrementalFuzzyIndex(const std::string& path,
                         boost::shared_ptr<IDManager>& idManager,
                         boost::shared_ptr<LAManager>& laManager,
                         IndexBundleSchema& indexSchema,
                         unsigned int Max_Doc_Num,
                         cma::Analyzer* &analyzer)
                        : idManager_(idManager)
                        , laManager_(laManager)
                        , indexSchema_(indexSchema)
                        , Max_Doc_Num_(Max_Doc_Num)
                        , analyzer_(analyzer)
    {
        pForwardIndex_  = NULL;
        pIncrementIndex_ = NULL;
        DocNumber_ = 0;
        doc_file_path_ = path;
        isAddedIndex_ = false;
    }

    bool IncrementalFuzzyIndex::buildIndex_(docid_t docId, std::string& text)
    {
        izenelib::util::UString utext(text, izenelib::util::UString::UTF_8);

        AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_unigram";
        LAInput laInput;
        laInput.setDocId(docId);
        if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
            return false;
        std::vector<uint32_t> termidList;
        std::vector<unsigned short> posList;
        for (LAInput::const_iterator it = laInput.begin(); it != laInput.end(); ++it)
        {
            termidList.push_back(it->termid_);
            posList.push_back(it->wordOffset_);
        }
        if (pForwardIndex_)
            pForwardIndex_->addDocForwardIndex_(docId, termidList, posList);
        else
            LOG(INFO) << "E:ForwardIndex is empty";

        if (pIncrementIndex_)
            pIncrementIndex_->addIndex_(docId, termidList , posList);
        else
            LOG(INFO) << "E:IncrementIndex is empty";
        return true;
    }

    bool IncrementalFuzzyIndex::score(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity)
    {
        Sentence querySentence(query.c_str());
        izenelib::util::ClockTimer timer;
        analyzer_->runWithSentence(querySentence);
        std::vector<std::vector<uint32_t> > termIN;
        std::vector<std::vector<uint32_t> > termNotIN;

        AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_unigram";

        for (int i = 0; i < querySentence.getCount(0); ++i)
        {
            //printf("%s, ", querySentence.getLexicon(0, i));
            string str(querySentence.getLexicon(0, i));
            LAInput laInput;
            laInput.setDocId(0);
            izenelib::util::UString utext(str, izenelib::util::UString::UTF_8);
            if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
                return false;
            std::vector<uint32_t> termidList;
            for (LAInput::const_iterator it = laInput.begin(); it != laInput.end(); ++it)
            {
                termidList.push_back(it->termid_);
            }
            termIN.push_back(termidList);
        }

        for (int i = 0; i < querySentence.getCount(1); i++)
        {
            //printf("%s, ", querySentence.getLexicon(1, i));
            string str(querySentence.getLexicon(1, i));
            izenelib::util::UString utext(str, izenelib::util::UString::UTF_8);
            LAInput laInput;
            laInput.setDocId(0);
            if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
                return false;
            std::vector<uint32_t> termidList;
            for (LAInput::const_iterator it = laInput.begin(); it != laInput.end(); ++it)
            {
                termidList.push_back(it->termid_);
            }
            termNotIN.push_back(termidList);
        }

        double rank = 0;
        for (uint32_t i = 0; i < resultList.size(); ++i)
        {
            rank = 0;
            for (std::vector<std::vector<uint32_t> >::iterator it = termIN.begin();
                    it != termIN.end(); ++it)
            {
                rank += getScore(*it, resultList[i])*double(2.0);
            }
            for (std::vector<std::vector<uint32_t> >::iterator it = termNotIN.begin();
                    it != termNotIN.end(); ++it)
            {
                rank += getScore(*it, resultList[i])*double(1.0);
            }
            ResultListSimilarity.push_back(rank);
        }
        return true;
    }

    IncrementalFuzzyIndex::~IncrementalFuzzyIndex()
    {
        if (pForwardIndex_)
        {
            delete pForwardIndex_;
            pForwardIndex_ = NULL;
        }

        if (pIncrementIndex_)
        {
            delete pIncrementIndex_;
            pIncrementIndex_ = NULL;
        }
    }

    void IncrementalFuzzyIndex::reset()
    {
        if (pForwardIndex_)
        {
            delete pForwardIndex_;
            pForwardIndex_ = NULL;
        }

        if (pIncrementIndex_)
        {
            delete pIncrementIndex_;
            pIncrementIndex_ = NULL;
        }
        bfs::path pathMainInc = doc_file_path_ + ".inc.idx";
        bfs::path pathMainFd = doc_file_path_ + ".fd.idx";

        bfs::remove(pathMainInc);
        bfs::remove(pathMainFd);
    }

    bool IncrementalFuzzyIndex::init(std::string& path)
    {
        pForwardIndex_ = new ForwardIndex(doc_file_path_, Max_Doc_Num_);
        pIncrementIndex_ = new InvertedIndex(doc_file_path_, Max_Doc_Num_, analyzer_);//
        if (pForwardIndex_ == NULL || pIncrementIndex_ == NULL)
        {
            return false;
        }
        return true;
    }

    bool IncrementalFuzzyIndex::load_(std::string path)
    {
        if (pForwardIndex_ != NULL && pIncrementIndex_ != NULL)
        {
            return pForwardIndex_->load_(path)&&pIncrementIndex_->load_(path);
        }
        else
        {
            LOG(INFO) << "Index is not exit!!"<<endl;
            return false;
        }
    }

    bool IncrementalFuzzyIndex::save_(std::string path)
    {
        if (pForwardIndex_ != NULL && pIncrementIndex_ != NULL)
        {
            if (isAddedIndex_ == true)
            {
                return pForwardIndex_->save_(path)&&pIncrementIndex_->save_(path);
            }
            return true;
        }
        else
        {
            LOG(INFO) << "Index is not exit!!"<<endl;
            return false;
        }
    }

    uint32_t IncrementalFuzzyIndex::getScore(std::vector<uint32_t> &v, uint32_t & docid)
    {
        if (pForwardIndex_)
        {
            return pForwardIndex_->getScore(v, docid);
        }
        return 0;
    }

    void IncrementalFuzzyIndex::setStatus()
    {
        isAddedIndex_ = true;
    }

    void IncrementalFuzzyIndex::resetStatus()
    {
        isAddedIndex_ = false;
    }

    void IncrementalFuzzyIndex::getFuzzyResult_(std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList,  std::vector<double>& ResultListSimilarity, uint32_t& hitdoc)//
    {

        std::vector<pair<uint32_t, unsigned short> >* v = NULL;
        pForwardIndex_->resetBitMapMatrix();

        izenelib::util::ClockTimer timerx;
        for (std::vector<uint32_t>::iterator i = termidList.begin(); i != termidList.end(); ++i)
        {
            pIncrementIndex_->getTermIdPos_(*i, v);
            if (v == NULL)
                return;
            pForwardIndex_->setPosition_(v);
        }

        std::vector<std::vector<pair<uint32_t, unsigned short> >* > ORResultList;
        pIncrementIndex_->getResultORFuzzy_(termidList, ORResultList);

        std::vector<uint32_t> newTermidList;
        pForwardIndex_->getLongestSubString_(newTermidList, ORResultList, hitdoc);

        std::vector<uint32_t >::iterator iter = std::unique(newTermidList.begin(), newTermidList.end());
        newTermidList.erase(iter, newTermidList.end());
        pIncrementIndex_->getResultAND_(newTermidList, resultList, ResultListSimilarity);
    }

    void IncrementalFuzzyIndex::getExactResult_(std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList,  std::vector<double>& ResultListSimilarity)
    {
        izenelib::util::ClockTimer timer;
        pIncrementIndex_->getResultAND_(termidList, resultList, ResultListSimilarity);
        LOG(INFO)<<"ResultList Number:"<<resultList.size()<< " time cost:"<< timer.elapsed()<<endl;
    }

    void IncrementalFuzzyIndex::print()
    {
        pForwardIndex_->print();
        pIncrementIndex_->print();
    }

    void IncrementalFuzzyIndex::prepare_index_()
    {
        pIncrementIndex_->prepare_index_();
    }

    ForwardIndex* IncrementalFuzzyIndex::getForwardIndex()
    {
        return pForwardIndex_;
    }

    InvertedIndex* IncrementalFuzzyIndex::getIncrementIndex()
    {
        return pIncrementIndex_;
    }
}
