#ifndef SF1R_B5MMANAGER_TOURPROCESSOR_H
#define SF1R_B5MMANAGER_TOURPROCESSOR_H 
#include <util/ustring/UString.h>
#include <product-manager/product_price.h>
#include "b5m_helper.h"
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
#include <document-manager/ScdDocument.h>

namespace sf1r {

	const static std::string SCD_SOURCE = "Source";
	const static std::string SCD_PRICE = "SalesPrice";
    const static std::string SCD_FROM_CITY = "FromCity";
	const static std::string SCD_TO_CITY = "ToCity";
	const static std::string SCD_DOC_ID = "DOCID";
	const static std::string SCD_TIME_PLAN = "TimePlan";
    const static std::string SCD_UUID = "uuid";
    class TourProcessor{
        typedef izenelib::util::UString UString;

        typedef std::pair<std::string, std::string> BufferKey;
        struct BufferValueItem
        {
            std::string from;
            std::string to;
            uint32_t days;
            double price;
			std::string docid;
			bool operator<(const BufferValueItem& another) const
            {
                return days<another.days;
            }
        };
        typedef std::vector<BufferValueItem> BufferValue;
        typedef boost::unordered_map<BufferKey, BufferValue> Buffer;
		typedef boost::unordered_map<std::string,ScdDocument*> DocHash;
		typedef boost::unordered_set<std::string> Set;
    public:
        TourProcessor();
		~TourProcessor();
        bool Generate(const std::string& scd_path, const std::string& mdb_instance);

    private:

        void Insert_(ScdDocument& doc);
        uint32_t ParseDays_(const std::string& sdays) const;
        void Finish_();
		
		/**init the b5mo path*/
		int InitializeOPath(const std::string &mdb_instance);
		/**init the b5mp path*/
		int InitializePPath(const std::string &mdb_instance);
		/**set uuid*/
		void SetUUID(ScdDocument&doc);

		/**merage o */
		void Merger();
		
		/**aggregate p information*/
		void AggregateP(const BufferValue&value);

    private:
        Buffer  buffer_;
		DocHash doc_hash_;
		boost::shared_ptr<ScdWriter> pwriter_;
	};
}

#endif


