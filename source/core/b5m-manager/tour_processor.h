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

		struct UnionBufferKey
		{
			std::vector<BufferKey> union_key_;

			bool operator<(const UnionBufferKey&key)const 
			{
				boost::unordered_set<BufferKey> key_set;
				for(std::vector<BufferKey>::const_iterator iter = union_key_.begin();
							iter != union_key_.end(); ++iter)
				{
					key_set.insert(*iter);
				}

				for(std::vector<BufferKey>::const_iterator iter = key.union_key_.begin();
							iter != key.union_key_.end(); ++iter)
				{
					if(key_set.find(*iter) != key_set.end())
					{
						//notice:equal key return false
						return false;
					}
				}
				return union_key_[0] < key.union_key_[0] ? true:false;
			}

			void push_back(const BufferKey&key)
			{
				union_key_.push_back(key);
			}

			size_t size()const
			{
				return union_key_.size();
			}
		};

        struct BufferValueItem
        {
            std::string from;
            std::string to;
            std::pair<uint32_t, uint32_t> days;
            double price;
            ScdDocument doc;
            bool bcluster;
            bool operator<(const BufferValueItem& another) const
            {
                return days<another.days;
            }
        };

        typedef std::vector<BufferValueItem> BufferValue;
        typedef std::map<UnionBufferKey, BufferValue> Buffer;
		typedef boost::unordered_set<std::string> Set;
        typedef BufferValue Group;

    public:
        TourProcessor();
		~TourProcessor();
        bool Generate(const std::string& scd_path, const std::string& mdb_instance);

    private:

        void Insert_(ScdDocument& doc);
        std::pair<uint32_t, uint32_t> ParseDays_(const std::string& sdays) const;
        void Finish_();
		
        void FindGroups_(BufferValue& value);
        void GenP_(Group& g, Document& doc) const;

		void GenerateUnionKey(UnionBufferKey&union_key,
							  const std::string&from_city,
							  const std::string& to_city)const;

    private:
        std::string m_;
        Buffer buffer_;
        boost::mutex mutex_;
    };
}

#endif


