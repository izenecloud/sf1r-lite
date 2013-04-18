#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <document-manager/DocumentManager.h>
#include <document-manager/DocContainer.h>
#include <document-manager/Document.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include <3rdparty/am/luxio/array.h>
#include <util/bzip.h>
#include <ir/index_manager/utility/BitVector.h>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <util/izene_serialization.h>
#include <compression/compressor.h>

using namespace std;
namespace bf = boost::filesystem;

namespace sf1r
{

class OldDocContainer
{
    typedef Lux::IO::Array containerType;

public:
    OldDocContainer(const std::string&path)
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

    ~OldDocContainer()
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

    bool get(const unsigned int docId, Document& doc)
    {
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

        izenelib::util::izene_deserialization<Document> izd((char*)p, nsz);
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

    docid_t getMaxDocId() const
    {
        return maxDocID_;
    }

private:
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
    // old data using LzoCompressor
    LzoCompressor compressor_;
};

}


int main(int argc, char * argv[])
{
    using namespace sf1r;
    try
    {
        if (argc < 2)
        {
            printf("please input the collection data base path!\n");
            return -1;
        }
        std::string col_data_base = argv[1];
        std::cout << "begin migrate the collection data : " << col_data_base << std::endl;
        std::string docdata_path = col_data_base + "/collection-data/default-collection-dir/dm/";
        std::string docdata_newpath = col_data_base + "/collection-data/default-collection-dir/dm_new/";
        {
            OldDocContainer oldDocMgr(docdata_path);

            bf::create_directories(docdata_newpath);
            DocContainer newDocMgr(docdata_newpath);
            if (!oldDocMgr.open())
            {
                std::cout << "open old data failed." << std::endl;
                return -1;
            }
            if(!newDocMgr.open())
            {
                std::cout << "open new data failed." << std::endl;
                return -1;
            }
            Document doc;
            for (size_t i = 0; i <= oldDocMgr.getMaxDocId(); ++i)
            {
                if( oldDocMgr.get(i, doc) )
                {
                    if (!newDocMgr.insert(i, doc))
                    {
                        std::cout << "doc insert failed : " << i << std::endl; 
                    }
                }
                if (i%10000==0)
                {
                    std::cout << " converted doc : " << i << std::endl;
                }
            }
            newDocMgr.flush();
        }

        std::string now = boost::lexical_cast<std::string>(time(NULL));

        bf::rename(docdata_path + "DocumentPropertyTable.data", docdata_path + "DocumentPropertyTable.data.old" + now);
        bf::rename(docdata_path + "DocumentPropertyTable.aidx", docdata_path + "DocumentPropertyTable.aidx.old" + now);
        bf::rename(docdata_path + "MaxDocID.xml", docdata_path + "MaxDocID.xml.old" + now);

        bf::rename(docdata_newpath + "DocumentPropertyTable.data", docdata_path + "DocumentPropertyTable.data");
        bf::rename(docdata_newpath + "DocumentPropertyTable.aidx", docdata_path + "DocumentPropertyTable.aidx");
        bf::rename(docdata_newpath + "MaxDocID.xml", docdata_path + "MaxDocID.xml");
        std::cout << "convert finished." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exit because of exception: "
                  << e.what() << std::endl;
    }

    return 1;
}
