#ifndef CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_PRUNED_SORTED_FORWARD_INDEX
#define CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_PRUNED_SORTED_FORWARD_INDEX

#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/AbsTermReader.h>
#include <ir/index_manager/index/AbsTermIterator.h>
#include <am/matrix/matrix_db.h>
#include <3rdparty/am/luxio/array.h>
#include <util/izene_serialization.h>


#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>

#include <vector>
#include <string>
#include <map>

namespace sf1r{

using izenelib::ir::indexmanager::IndexReader;
using izenelib::ir::indexmanager::TermReader;
using izenelib::ir::indexmanager::TermIterator;

typedef std::vector<std::pair<uint32_t, float> > PrunedForwardType;

class PrunedForwardContainer
{
    typedef Lux::IO::Array containerType;
public:
    PrunedForwardContainer(const std::string&path) :
            path_(path),
            fileName_(boost::filesystem::path(boost::filesystem::path(path)/"PrunedForward").string()),
            maxDocIdDb_(boost::filesystem::path(boost::filesystem::path(path)/"MaxDocID.xml").string()),
            containerPtr_(NULL),
            maxDocID_(0)
    {
        containerPtr_ = new containerType(Lux::IO::NONCLUSTER);
        containerPtr_->set_noncluster_params(Lux::IO::Linked);
        restoreMaxDocDb_();
    }
    ~PrunedForwardContainer()
    {
        if (containerPtr_)
        {
            containerPtr_->close();
            delete containerPtr_;
        }
    }
    bool open()
    {
        try
        {
            if ( !boost::filesystem::exists(fileName_) )
            {
                containerPtr_->open(fileName_.c_str(), Lux::IO::DB_CREAT);
            }
            else
            {
                containerPtr_->open(fileName_.c_str(), Lux::IO::DB_RDWR);
            }
        }
        catch (...)
        {
            return false;
        }
        return true;

    }
    bool insert(const unsigned int docId, const PrunedForwardType& forward)
    {
        maxDocID_ = docId>maxDocID_? docId:maxDocID_;
        izenelib::util::izene_serialization<PrunedForwardType> izs(forward);
        char* src;
        size_t srcLen;
        izs.write_image(src, srcLen);
        return containerPtr_->put(docId, src, srcLen, Lux::IO::APPEND);
    }
    bool get(const unsigned int docId, PrunedForwardType& forward)
    {
        if (docId > maxDocID_ )
            return false;
        Lux::IO::data_t *val_p = NULL;
        if (false == containerPtr_->get(docId, &val_p, Lux::IO::SYSTEM))
        {
            containerPtr_->clean_data(val_p);
            return false;
        }
        izenelib::util::izene_deserialization<PrunedForwardType> izd((char*)(val_p->data), val_p->size);
        izd.read_image(forward);
        containerPtr_->clean_data(val_p);
        return true;
    }

    bool del(const unsigned int docId)
    {
        if (docId > maxDocID_ )
            return false;
        return containerPtr_->del(docId);
    }

    void flush()
    {
        saveMaxDocDb_();
    }

private:
    bool saveMaxDocDb_() const
    {
        try
        {
            std::ofstream ofs(maxDocIdDb_.c_str());
            if (ofs)
            {
                boost::archive::xml_oarchive oa(ofs);
                oa << boost::serialization::make_nvp(
                    "MaxDocID", maxDocID_
                );
            }

            return ofs;
        }
        catch (boost::archive::archive_exception& e)
        {
            return false;
        }
    }

    bool restoreMaxDocDb_()
    {
        try
        {
            std::ifstream ifs(maxDocIdDb_.c_str());
            if (ifs)
            {
                boost::archive::xml_iarchive ia(ifs);
                ia >> boost::serialization::make_nvp(
                    "MaxDocID", maxDocID_
                );
            }
            return ifs;
        }
        catch (boost::archive::archive_exception& e)
        {
            maxDocID_ = 0;
            return false;
        }
    }
private:
    std::string path_;
    std::string fileName_;
    std::string maxDocIdDb_;
    containerType* containerPtr_;
    uint32_t maxDocID_;
};

class PrunedSortedForwardIndex : public boost::noncopyable
{
public:
    PrunedSortedForwardIndex(
        const std::string& workingDir,
        IndexReader* reader,
        unsigned int prunedBound,
        unsigned int docNum,
        unsigned int maxDoc
    );

    ~PrunedSortedForwardIndex();

    void addField(const std::pair<std::string, uint32_t>& field, float fieldWeight, float avgDocLen)
    {
        fields_[field] = std::make_pair(fieldWeight,avgDocLen);
    }

    void build();

private:

    void buildForward_();

    void buildPrunedForward_();

private:
    std::string workingDir_;
    IndexReader* indexReader_;
    unsigned int prunedBound_;
    std::string forwardIndexPath_;
    izenelib::am::MatrixDB<uint32_t, float> forwardIndex_;
    boost::scoped_ptr<TermReader> termReader_;
    std::map<std::pair<std::string, uint32_t>, std::pair<float,float> > fields_;

    PrunedForwardContainer prunedForwardContainer_;

    size_t numDocs_;
    uint32_t maxDoc_;

    static const float default_k1_=1.2;
    static const float default_b_=0.75;
    friend class SemanticKernel;
};

}

#endif
