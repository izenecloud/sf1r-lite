#ifndef DOCCONTAINER_H_
#define DOCCONTAINER_H_

#include <common/PropertyKey.h>
#include <common/PropertyValue.h>

#include "Document.h"

#include <3rdparty/am/luxio/array.h>
#include <3rdparty/am/stx/btree_set.h>
#include <util/izene_serialization.h>
#include <util/bzip.h>

#include <ir/index_manager/utility/BitVector.h>

#include <boost/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/scoped_array.hpp>

#include <compression/minilzo/minilzo.h>
#define OUTPUT_BLOCK_SIZE(input_block_size) (input_block_size + (input_block_size / 16) + 64 + 3)

namespace sf1r
{

class DocContainer
{
    typedef Lux::IO::Array containerType;
public:
    DocContainer(const std::string&path, const std::vector<std::string>& snippetPropertyList) :
            path_(path),
            fileName_(path + "DocumentPropertyTable"),
            maxDocIdDb_(path + "MaxDocID.xml"),
            containerPtr_(NULL),
            snippetPropertyList_(snippetPropertyList),
            maxDocID_(0)
    {
        containerPtr_ = new containerType(Lux::IO::NONCLUSTER);
        containerPtr_->set_noncluster_params(Lux::IO::Linked);
        restoreMaxDocDb_();

        propertyContainerPtr_.resize(snippetPropertyList_.size());
        for(size_t i = 0; i < snippetPropertyList_.size(); i++)
        {
            propertyContainerPtr_[i] = new containerType(Lux::IO::NONCLUSTER);
            propertyContainerPtr_[i]->set_noncluster_params(Lux::IO::Linked);
        }
    }
    ~DocContainer()
    {
        if (containerPtr_)
        {
            containerPtr_->close();
            delete containerPtr_;
        }
        if (!propertyContainerPtr_.empty())
        {
            for(size_t i = 0 ; i < propertyContainerPtr_.size(); i++)
            {
                if(propertyContainerPtr_[i])
                {
                    propertyContainerPtr_[i]->close();
                    delete propertyContainerPtr_[i];
                }
            }
            propertyContainerPtr_.clear();
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

            for ( size_t i = 0; i < snippetPropertyList_.size(); i++)
            {
                if ( !boost::filesystem::exists(snippetPropertyList_[i]) )
                {
                    propertyContainerPtr_[i]->open( (path_ + snippetPropertyList_[i]).c_str(), Lux::IO::DB_CREAT );
                }
                else
                {
                    propertyContainerPtr_[i]->open( (path_ + snippetPropertyList_[i]).c_str(), Lux::IO::DB_RDWR );
                }
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

        const size_t allocSize = OUTPUT_BLOCK_SIZE(srcLen);
        size_t destLen = 0;
        boost::scoped_array<unsigned char> destPtr(new unsigned char[allocSize + sizeof(uint32_t)]);
        *reinterpret_cast<uint32_t*>(destPtr.get()) = srcLen;

        START_PROFILER( proDocumentCompression )
        if (compress_(src, srcLen, destPtr.get() + sizeof(uint32_t), allocSize, destLen) == false)
            return false;
        STOP_PROFILER( proDocumentCompression )

        bool ret = containerPtr_->put(docId, destPtr.get(), destLen + sizeof(uint32_t), Lux::IO::APPEND);
        if (ret)
        {
            unsigned int id = docId;
            containerPtr_->put(0, &id, sizeof(unsigned int), Lux::IO::OVERWRITE);
        }

        bool isPropertyInserted = insertPropertyValue(docId, doc);
        return ret & isPropertyInserted;
    }
    bool get(const unsigned int docId, Document& doc)
    {
        //CREATE_SCOPED_PROFILER(get_document, "Index:SIAProcess", "Indexer : get_document")
        //CREATE_PROFILER(proDocumentDecompression, "Index:SIAProcess", "Indexer : DocumentDecompression")
        if (docId > maxDocID_ )
            return false;
        Lux::IO::data_t *val_p = NULL;
        if (false == containerPtr_->get(docId, &val_p, Lux::IO::SYSTEM))
        {
            containerPtr_->clean_data(val_p);
            return false;
        }
        int nsz=0;

        //START_PROFILER(proDocumentDecompression)

        const uint32_t allocSize = *reinterpret_cast<const uint32_t*>(val_p->data);
        unsigned char* p = new unsigned char[allocSize];
        lzo_uint tmpTarLen;
        int re = lzo1x_decompress((const unsigned char*)val_p->data + sizeof(uint32_t), val_p->size - sizeof(uint32_t), p, &tmpTarLen, NULL);
        //int re = bmz_unpack(val_p->data, val_p->size, p, &tmpTarLen, NULL);

        if (re != LZO_E_OK || tmpTarLen != allocSize)
            return false;

        nsz = tmpTarLen; //
        //char *p =(char*)_tc_bzdecompress((const char*)val_p->data, val_p->size, &nsz);
        //STOP_PROFILER(proDocumentDecompression)

        izene_deserialization<Document> izd((char*)p, nsz);
        izd.read_image(doc);
        delete[] p;
        containerPtr_->clean_data(val_p);
        return true;
    }

    bool getPropertyValue(const unsigned int docId, int index, PropertyValue& result)
    {
        if (docId > maxDocID_)
            return false;
        Lux::IO::data_t *val_p = NULL;
        if (false == propertyContainerPtr_[index]->get(docId, &val_p, Lux::IO::SYSTEM))
        {
            propertyContainerPtr_[index]->clean_data(val_p);
            return false;
        }
        int nsz=0;

        const uint32_t allocSize = *reinterpret_cast<const uint32_t*>(val_p->data);
        unsigned char* p = new unsigned char[allocSize];
        lzo_uint tmpTarLen;
        int re = lzo1x_decompress((const unsigned char*)val_p->data + sizeof(uint32_t), val_p->size - sizeof(uint32_t), p, &tmpTarLen, NULL);

        if (re != LZO_E_OK || tmpTarLen != allocSize)
            return false;

        nsz = tmpTarLen; //

        izene_deserialization<PropertyValue> izd((char*)p, nsz);
        izd.read_image(result);
        delete[] p;
        propertyContainerPtr_[index]->clean_data(val_p);
        return true;
    }

    bool del(const unsigned int docId)
    {
        if (docId > maxDocID_ )
            return false;

        return (containerPtr_->del(docId)) & (delProperty(docId));
    }
    bool update(const unsigned int docId, const Document& doc)
    {
        if (docId > maxDocID_)
            return false;

        izene_serialization<Document> izs(doc);
        char* src;
        size_t srcLen;
        izs.write_image(src, srcLen);

        const size_t allocSize = OUTPUT_BLOCK_SIZE(srcLen);
        size_t destLen = 0;
        boost::scoped_array<unsigned char> destPtr(new unsigned char[allocSize + sizeof(uint32_t)]);
        *reinterpret_cast<uint32_t*>(destPtr.get()) = srcLen;

        if (compress_(src, srcLen, destPtr.get() + sizeof(uint32_t), allocSize, destLen) == false)
            return false;

        bool ret = containerPtr_->put(docId, destPtr.get(), destLen + sizeof(uint32_t), Lux::IO::OVERWRITE);

        bool isPropertyUpdated = updatePropertyValue(docId, doc);
        return ret & isPropertyUpdated;
    }

    int getPropertyIndex(const std::string& propertyName)
    {
        size_t pos = 0, i = 0;
        for(; i < snippetPropertyList_.size(); i++)
        {
            if (snippetPropertyList_[i] == propertyName)
            {
                pos = i;
                break;
            }
        }

        if( i == snippetPropertyList_.size() )
        {
            return -1;
        }
        else
        {
            return pos;
        }
    }

    docid_t getMaxDocId()
    {
        return maxDocID_;
    }

    void flush()
    {
        saveMaxDocDb_();
    }

private:
    bool insertPropertyValue(const unsigned int docId, const Document& doc)
    {
        for( size_t i = 0; i < snippetPropertyList_.size(); i++)
        {
            const std::string& propertyName = snippetPropertyList_[i];
            const PropertyValue value = doc.property(propertyName);
            izene_serialization<PropertyValue> izs(value);
            char* src;
            size_t srcLen;
            izs.write_image(src, srcLen);

            const size_t allocSize = OUTPUT_BLOCK_SIZE(srcLen);
            size_t destLen = 0;
            boost::scoped_array<unsigned char> destPtr(new unsigned char[allocSize + sizeof(uint32_t)]);
            *reinterpret_cast<uint32_t*>(destPtr.get()) = srcLen;

            if (compress_(src, srcLen, destPtr.get() + sizeof(uint32_t), allocSize, destLen) == false)
                return false;

            bool ret = propertyContainerPtr_[i]->put(docId, destPtr.get(), destLen + sizeof(uint32_t), Lux::IO::APPEND);
            if (ret)
            {
                unsigned int id = docId;
                propertyContainerPtr_[i]->put(0, &id, sizeof(unsigned int), Lux::IO::OVERWRITE);
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    bool updatePropertyValue(const unsigned int docId, const Document& doc)
    {
        for( size_t i = 0; i < snippetPropertyList_.size(); i++)
        {
            const std::string& propertyName = snippetPropertyList_[i];
            const PropertyValue& value = doc.property(propertyName);
            izene_serialization<PropertyValue> izs(value);
            char* src;
            size_t srcLen;
            izs.write_image(src, srcLen);

            const size_t allocSize = OUTPUT_BLOCK_SIZE(srcLen);
            size_t destLen = 0;
            boost::scoped_array<unsigned char> destPtr(new unsigned char[allocSize + sizeof(uint32_t)]);
            *reinterpret_cast<uint32_t*>(destPtr.get()) = srcLen;

            if (compress_(src, srcLen, destPtr.get() + sizeof(uint32_t), allocSize, destLen) == false)
                return false;

            bool ret = propertyContainerPtr_[i]->put(docId, destPtr.get(), destLen + sizeof(uint32_t), Lux::IO::OVERWRITE);
            if(!ret)
                return false;
        }
        return true;
    }

    bool delProperty(const unsigned int docId)
    {
        for(size_t i = 0; i < propertyContainerPtr_.size(); i++)
        {
            if ( !propertyContainerPtr_[i]->del(docId) )
                return false;
        }
        return true;
    }

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

    bool compress_(char* src, size_t srcLen, unsigned char* dest, size_t destAllocSize, size_t& destLen)
    {
        static lzo_align_t __LZO_MMODEL
        wrkmem [((LZO1X_1_MEM_COMPRESS)+(sizeof(lzo_align_t)-1))/sizeof(lzo_align_t)];

        lzo_uint tmpTarLen;
        int re = lzo1x_1_compress((unsigned char*)src, srcLen, dest, &tmpTarLen, wrkmem);
        if ( re != LZO_E_OK || tmpTarLen > destAllocSize )
            return false;

        destLen = tmpTarLen;
        return true;
    }

private:
    std::string path_;
    std::string fileName_;
    std::string maxDocIdDb_;
    containerType* containerPtr_;
    std::vector<std::string> snippetPropertyList_;
    std::vector<containerType*> propertyContainerPtr_;
    docid_t maxDocID_;
};

}
#endif /*DOCCONTAINER_H_*/
