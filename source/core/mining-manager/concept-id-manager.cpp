#include "concept-id-manager.h"

using namespace sf1r;

ConceptIDManager::ConceptIDManager(const std::string& dir)
        :isOpen_(false),idMap_(dir+"/id_map"),save_existence_(false),existence_(dir+"/existence")
{
    RegisterErrorMsg_(1, "file io error");
}


ConceptIDManager::~ConceptIDManager()
{
    Close();
}

void ConceptIDManager::SetSaveExistence(bool b)
{
    save_existence_ = b;
}

bool ConceptIDManager::Open()
{
    try
    {
        idMap_.setCacheSize(10000000);
        idMap_.open();
        if ( save_existence_ )
        {
            existence_.setCacheSize(10000);
            existence_.open();
        }
    }
    catch (std::exception& ex)
    {
        ReportError_(1);
        return false;
    }
    isOpen_ = true;
    return true;
}

bool ConceptIDManager::IsOpen() const
{
    return isOpen_;
}



void ConceptIDManager::Put(uint32_t id, const izenelib::util::UString& ustr)
{
    if (id==0 ) return;

    idMap_.update(id, ustr);
    if ( save_existence_ )
    {
        boost::mutex::scoped_lock lock(mutex_);
        existence_.update(Hash(ustr), id);
    }


}

bool ConceptIDManager::GetIdByString(const izenelib::util::UString& ustr, uint32_t& id)
{
    if ( !save_existence_ ) return false;
    boost::mutex::scoped_lock lock(mutex_);
    uint64_t hashId = izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(ustr);
    return existence_.get(hashId, id);

}

bool ConceptIDManager::GetStringById(uint32_t id, izenelib::util::UString& ustr)
{
    return idMap_.get(id, ustr);
}


void ConceptIDManager::Flush()
{
    boost::mutex::scoped_lock lock(mutex_);
    idMap_.flush();
    if ( save_existence_ )
    {
        existence_.flush();
    }
}

void ConceptIDManager::Close()
{
    boost::mutex::scoped_lock lock(mutex_);
    if (IsOpen())
    {
        idMap_.close();
        if ( save_existence_ )
        {
            existence_.close();
        }
        isOpen_ = false;
    }
}

size_t ConceptIDManager::size()
{
    return idMap_.getItemCount();
}
