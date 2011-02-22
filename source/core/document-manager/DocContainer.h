#ifndef DOCCONTAINER_H_
#define DOCCONTAINER_H_

#include "Document.h"

#include <3rdparty/am/luxio/array.h>
#include <3rdparty/am/stx/btree_set.h>
#include <util/izene_serialization.h>
#include <util/bzip.h>

#include <ir/index_manager/utility/BitVector.h>
#include <boost/memory.hpp>

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/assert.hpp>

#include <compression/minilzo/minilzo.h>
//#include "bmz.h"


namespace sf1r {

class DocContainer {
	typedef Lux::IO::Array containerType;
public:
	DocContainer(const std::string&path) :
		path_(path),
		fileName_(path + "DocumentPropertyTable"),
		maxDocIdDb_(path + "MaxDocID.xml"){
		containerPtr_ = new containerType(Lux::IO::NONCLUSTER);
		containerPtr_->set_noncluster_params(Lux::IO::Linked);
		restoreMaxDocDb_();
	}
	~DocContainer() {
		if (containerPtr_)
		{
			containerPtr_->close();
			delete containerPtr_;
		}
	}
	bool open() {
		try {
			if ( !boost::filesystem::exists(fileName_) ) {
				containerPtr_->open(fileName_.c_str(), Lux::IO::DB_CREAT);
			} else {
				containerPtr_->open(fileName_.c_str(), Lux::IO::DB_RDWR);
			}
		} catch(...) {
			return false;
		}
		return true;

	}
	bool insert(const unsigned int docId, const Document& doc) {
		CREATE_SCOPED_PROFILER(insert_document, "Index:SIAProcess", "Indexer : insert_document")
		CREATE_PROFILER(proDocumentCompression, "Index:SIAProcess", "Indexer : DocumentCompression")
		maxDocID_ = docId>maxDocID_? docId:maxDocID_;
		izene_serialization<Document> izs(doc);
		char* ptr;
		size_t sz;
		izs.write_image(ptr, sz);
		
		int nsz=0;		
		START_PROFILER( proDocumentCompression )
			
		static lzo_align_t __LZO_MMODEL
		wrkmem [((LZO1X_1_MEM_COMPRESS)+(sizeof(lzo_align_t)-1))/sizeof(lzo_align_t)];
		//  void* wrkmem = new char[sz*2];

		lzo_uint tmpTarLen;
		//unsigned char* p = new unsigned char[sz];	
		boost::scoped_alloc alloc(recycle_);
		unsigned char* p = BOOST_NEW_ARRAY(alloc, unsigned char, sz + sizeof(uint32_t));
                *reinterpret_cast<uint32_t*>(p) = sz;

		int re = lzo1x_1_compress((unsigned char*)ptr, sz, p + sizeof(uint32_t), &tmpTarLen, wrkmem);
		//int re = bmz_pack(ptr, sz, p, &tmpTarLen, 0, 50, (1 << 24), wrkmem);
		if ( re != LZO_E_OK )
			return false;
		nsz = tmpTarLen;
		//char *p =(char*)_tc_bzcompress(ptr, sz, &nsz);
		
		STOP_PROFILER( proDocumentCompression ) 
		
		bool ret = containerPtr_->put(docId, p, nsz + sizeof(uint32_t), Lux::IO::APPEND);
		//delete []wrkmem;
		//delete [] p;
		if(ret)
		{
			unsigned int id = docId;
			containerPtr_->put(0, &id, sizeof(unsigned int), Lux::IO::OVERWRITE);
		}
		return ret;
	}
	bool get(const unsigned int docId, Document& doc) {
		//CREATE_SCOPED_PROFILER(get_document, "Index:SIAProcess", "Indexer : get_document")
		//CREATE_PROFILER(proDocumentDecompression, "Index:SIAProcess", "Indexer : DocumentDecompression")
		if (docId > maxDocID_ )
			return false;
		Lux::IO::data_t val_data;
		Lux::IO::data_t *val_p = &val_data;
		if(false == containerPtr_->get(docId, &val_p, Lux::IO::SYSTEM))
		{
			containerPtr_->clean_data(val_p);
			return false;
		}
		int nsz=0;
		
		//START_PROFILER(proDocumentDecompression)

                const uint32_t allocSize = *reinterpret_cast<const uint32_t*>(val_p->data);
		boost::scoped_alloc alloc;
		//unsigned char* p = new unsigned char[allocSize];
		unsigned char* p = BOOST_NEW_ARRAY(alloc, unsigned char, allocSize);
		lzo_uint tmpTarLen;
		int re = lzo1x_decompress((const unsigned char*)val_p->data + sizeof(uint32_t), val_p->size - sizeof(uint32_t), p, &tmpTarLen, NULL);
		//int re = bmz_unpack(val_p->data, val_p->size, p, &tmpTarLen, NULL);

		if (re != LZO_E_OK)
			return false;

                BOOST_ASSERT(tmpTarLen == allocSize);

		nsz = tmpTarLen; //
		//char *p =(char*)_tc_bzdecompress((const char*)val_p->data, val_p->size, &nsz);
		//STOP_PROFILER(proDocumentDecompression)
		
		izene_deserialization<Document> izd((char*)p, nsz);
		izd.read_image(doc);
		//delete[] p;
		containerPtr_->clean_data(val_p);
		return true;
	}

	bool del(const unsigned int docId) {
		if (docId > maxDocID_ )
			return false;
		return containerPtr_->del(docId);
	}
	bool update(const unsigned int docId, const Document& doc) {
		if(docId > maxDocID_) return false;
		izene_serialization<Document> izs(doc);
		char* ptr;
		size_t sz;
		izs.write_image(ptr, sz);
		
		int nsz=0;
		char *p =(char*)_tc_bzcompress(ptr, sz, &nsz);
		bool ret = containerPtr_->put(docId, p, nsz, Lux::IO::OVERWRITE);
		delete p;
		return ret;
	}

	docid_t getMaxDocId() {
		return maxDocID_;
	}

	void flush() 
	{
		saveMaxDocDb_();
	}

private:
	bool saveMaxDocDb_() const {
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
		catch(boost::archive::archive_exception& e)
		{
			return false;
		}
	}
	
	bool restoreMaxDocDb_() {
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
		catch(boost::archive::archive_exception& e)
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
	
	NS_BOOST_MEMORY::block_pool recycle_;
};

}
#endif /*DOCCONTAINER_H_*/
