#ifndef SF1V5_DOCUMENT_MANAGER_DOC_CONTAINER_H
#define SF1V5_DOCUMENT_MANAGER_DOC_CONTAINER_H

#include "Document.h"

#include <3rdparty/am/luxio/array.h>
#include <util/izene_serialization.h>
#include <util/bzip.h>

#include <ir/index_manager/utility/BitVector.h>

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/scoped_array.hpp>

#include <compression/compressor.h>

namespace sf1r
{

class DocContainer
{
    typedef Lux::IO::Array containerType;

public:
    DocContainer(const std::string&path)
        : path_(path)
        , fileName_(path + "DocumentPropertyTable")
        , maxDocIdDb_(path + "MaxDocID.xml")
        , containerPtr_(NULL)
        , maxDocID_(0)
    {
        containerPtr_ = new containerType(Lux::IO::NONCLUSTER);
        containerPtr_->set_noncluster_params(Lux::IO::Linked);
        containerPtr_->set_lock_type(Lux::IO::LOCK_THREAD);
        restoreMaxDocDb_();
    }

    ~DocContainer()
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

    bool insert(const unsigned int docId, const Document& doc)
    {
        CREATE_SCOPED_PROFILER(insert_document, "Index:SIAProcess", "Indexer : insert_document")
        CREATE_PROFILER(proDocumentCompression, "Index:SIAProcess", "Indexer : DocumentCompression")
        maxDocID_ = docId>maxDocID_? docId:maxDocID_;
        izene_serialization<Document> izs(doc);
        char* src;
        size_t srcLen;
        izs.write_image(src, srcLen);

        const size_t allocSize = compressor_.compressBound(srcLen);
        size_t destLen = 0;
        boost::scoped_array<unsigned char> destPtr(new unsigned char[allocSize + sizeof(uint32_t)]);
        *reinterpret_cast<uint32_t*>(destPtr.get()) = srcLen;

        if ( srcLen == 0 )
        {
            return false;
        }

        START_PROFILER( proDocumentCompression )
        bool re = compressor_.compress((const unsigned char*)src, srcLen, destPtr.get() + sizeof(uint32_t), destLen);
        if (!re || destLen > allocSize)
            return false;
        STOP_PROFILER( proDocumentCompression )

        bool ret = containerPtr_->put(docId, destPtr.get(), destLen + sizeof(uint32_t), Lux::IO::OVERWRITE);
        return ret;
    }

    bool get(const unsigned int docId, Document& doc)
    {
        //CREATE_SCOPED_PROFILER(get_document, "Index:SIAProcess", "Indexer : get_document")
        //CREATE_PROFILER(proDocumentDecompression, "Index:SIAProcess", "Indexer : DocumentDecompression")
        if (docId > maxDocID_ )
        {
            return false;
        }
        Lux::IO::data_t *val_p = NULL;
        if (!containerPtr_->get(docId, &val_p, Lux::IO::SYSTEM))
        {
            containerPtr_->clean_data(val_p);
            return false;
        }
        int nsz=0;

        //START_PROFILER(proDocumentDecompression)

        if ( val_p->size - sizeof(uint32_t) == 0 )
        {
            containerPtr_->clean_data(val_p);
            return false;
        }

        const uint32_t allocSize = *reinterpret_cast<const uint32_t*>(val_p->data);
        unsigned char* p = new unsigned char[allocSize];
        bool re = compressor_.decompress((const unsigned char*)val_p->data + sizeof(uint32_t), val_p->size - sizeof(uint32_t), p, allocSize);

        if (!re)
        {
            delete[] p;
            containerPtr_->clean_data(val_p);
            return false;
        }

        nsz = allocSize; //
        //STOP_PROFILER(proDocumentDecompression)

        izene_deserialization<Document> izd((char*)p, nsz);
        izd.read_image(doc);
        delete[] p;
        containerPtr_->clean_data(val_p);
        return true;
    }

    bool exist(const unsigned int docId)
    {
        if (docId > maxDocID_ )
            return false;
        Lux::IO::data_t *val_p = NULL;
        bool ret = containerPtr_->get(docId, &val_p, Lux::IO::SYSTEM);
        containerPtr_->clean_data(val_p);
        return ret;
    }

    bool del(const unsigned int docId)
    {
        if (docId > maxDocID_ )
            return false;
        return containerPtr_->del(docId);
    }

    bool update(const unsigned int docId, const Document& doc)
    {
        if (docId > maxDocID_)
            return false;

        izene_serialization<Document> izs(doc);
        char* src;
        size_t srcLen;
        izs.write_image(src, srcLen);

        const size_t allocSize = compressor_.compressBound(srcLen);
        size_t destLen = 0;
        boost::scoped_array<unsigned char> destPtr(new unsigned char[allocSize + sizeof(uint32_t)]);
        *reinterpret_cast<uint32_t*>(destPtr.get()) = srcLen;

        if ( srcLen == 0)
        {
            return false;
        }
        bool re = compressor_.compress((const unsigned char*)src, srcLen, destPtr.get() + sizeof(uint32_t), destLen);
        if (!re || destLen > allocSize)
            return false;

        bool ret = containerPtr_->put(docId, destPtr.get(), destLen + sizeof(uint32_t), Lux::IO::OVERWRITE);
        return ret;
    }

    docid_t getMaxDocId() const
    {
        return maxDocID_;
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
            ///Not Used. Array[0] could be used to store maxDocID since all doc ids start from 1
            containerPtr_->put(0, &maxDocID_, sizeof(unsigned int), Lux::IO::OVERWRITE);
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
    docid_t maxDocID_;
    DocumentCompressor compressor_;
};

}
#endif /*DOCCONTAINER_H_*/
