///
/// @file PropertyExpr.h
/// @brief header file of PropertyExpr
/// @author Dohyun YUn
/// @date 2008-12-10
///

#include "PropertyTermInfo.h"
#include <glog/logging.h>

using namespace std;

namespace sf1r {

    PropertyTermInfo::PropertyTermInfo(void) : isWildCardQuery_(false)
    {
    } // end - PropertyTermInfo()

    PropertyTermInfo::~PropertyTermInfo(void)
    {
    } // end - ~PropertyTermInfo()

    PropertyTermInfo& PropertyTermInfo::operator=(const PropertyTermInfo& other)
    {
        searchProperty_ = other.searchProperty_;

        termIdPositionMap_ = other.termIdPositionMap_ ;

        termIdFrequencyMap_ = other.termIdFrequencyMap_;

        termIdKeywordMap_ = other.termIdKeywordMap_;

        isWildCardQuery_ = other.isWildCardQuery_;

        return *this;
    }

    void PropertyTermInfo::setSearchProperty(const string& searchProperty)
    {
        searchProperty_ = searchProperty;
    }

    const string& PropertyTermInfo::getSearchProperty(void) const
    {
        return searchProperty_;
    }

    void PropertyTermInfo::dealWithTerm(izenelib::util::UString& keyword, termid_t termId, unsigned int nPos)
    {
        id_uint_list_map_t::iterator termIdPositionPos;
        ID_FREQ_MAP_T::iterator termIdFrequencyPos;

        // insert keyword into termIdKeywordMap_
        termIdKeywordMap_.insert( make_pair(termId , keyword) );

        // insert position information of given term into position list.
        termIdPositionPos = termIdPositionMap_.find( termId );
        if( termIdPositionPos == termIdPositionMap_.end() ) // if it is not inserted yet.
        {
            std::string keywordString;
            keyword.convertString( keywordString , izenelib::util::UString::UTF_8 );
            
            // DLOG(INFO) << "[PropertyTermInfo] new term id is inserted" << endl
            //     << "(Term : " << keywordString << " , " << termId 
            //     << ") (pos : " << nPos << ") (freq : 1)" << endl;

            std::list<unsigned int> sequenceList;
            sequenceList.push_back( nPos );
            termIdPositionMap_.insert( std::make_pair( termId, sequenceList ) );
            termIdFrequencyMap_.insert( std::make_pair( termId, (float)1 ) );
        }
        else // already exist 
        {
            termIdPositionPos->second.push_back( nPos );

            termIdFrequencyPos = termIdFrequencyMap_.find( termId );
            if( termIdFrequencyPos != termIdFrequencyMap_.end() )
                termIdFrequencyPos->second += (float)1;
            else
            {
                DLOG(ERROR) << "[PropertyTermInfo] term id is not found in frequency map" << endl;
                return;
            }

            // DLOG(INFO) << "[PropertyTermInfo] term id is exist" << endl
            //     << "(TermId : " << termId << ") (pos : " << nPos 
            //     << ") (freq : " << termIdFrequencyPos->second<< ")" << endl;
        }

    } // end - dealWithTerm()

    std::string PropertyTermInfo::toString(void) const
    {
        stringstream ss;
        ss << "PropertyTermInfo" << endl;
        ss << "----------------------------------" << endl;
        ss << "Property : " << searchProperty_ << endl;
        ss << "TermIdPositionmap : " << endl;;
        for( id_uint_list_map_t::const_iterator iter = termIdPositionMap_.begin();
                iter != termIdPositionMap_.end(); iter++)
        {
            ss << "(" << iter->first << " : ";
            for( std::list<unsigned int>::const_iterator iter2 = iter->second.begin();
                    iter2 != iter->second.end(); iter2++)
                ss << *iter2 << " ";
            ss << endl;
        } // end - for
        ss << endl << "TermIdFrequency : " << endl;
        for(ID_FREQ_MAP_T::const_iterator iter =  termIdFrequencyMap_.begin();
                iter != termIdFrequencyMap_.end(); iter++)
        {
            ss << "(" << iter->first << " , " << iter->second << ") ";
        } // end - for
        ss << endl << "----------------------------------" << endl;
        return ss.str();
    } // end - print()

    void PropertyTermInfo::getPositionedTermIdList(std::vector<termid_t>& termIdList) const
    {
        // Copy termid-list<offset> map to termoffset-id map
        std::map<unsigned int, termid_t> termPositionIdMap;

        id_uint_list_map_t::const_iterator iter 
            = termIdPositionMap_.begin();

        unsigned int maxOffset = 0;

        for(; iter != termIdPositionMap_.end(); iter++)
        {
            std::list<unsigned int>::const_iterator offsetIter = iter->second.begin();
            for(; offsetIter != iter->second.end(); offsetIter++)
            {
                unsigned int offset = *offsetIter;
                std::pair<std::map<unsigned int, termid_t>::iterator , bool> ret
                 = termPositionIdMap.insert(make_pair(offset,iter->first) );
                if ( !ret.second )
                {
                    DLOG(ERROR) << "[PropertyTermInfo] Error Line : "<< __LINE__ 
                        << " - position value is duplicated (pos : " << *offsetIter 
                        << ", termid : " << iter->first << ")" << endl;
                    return;
                }

                if( offset > maxOffset )
                    maxOffset = offset;
            } // end - end of the list
        } // end - for end  of the termid

        // Copy termoffset-id map to termIdList.
        termIdList.clear();

        for(unsigned int offset=0; offset<=maxOffset; ++offset )
        {
            std::map<unsigned int, termid_t>::iterator utidItr = termPositionIdMap.find( offset );
            if( utidItr != termPositionIdMap.end() )
                termIdList.push_back( utidItr->second );
        } // end - for

    } // end - getPositionedTermIdList()

    void PropertyTermInfo::getPositionedTermStringList(std::vector<izenelib::util::UString>& termStringList) const
    {
        std::vector<termid_t> termIdList;
        this->getPositionedTermIdList( termIdList );

        termStringList.clear();
        std::vector<termid_t>::const_iterator iter=termIdList.begin();
        for(; iter != termIdList.end(); iter++)
        {
            std::map<termid_t, izenelib::util::UString>::const_iterator termStringIter = termIdKeywordMap_.find( *iter );
            if ( termStringIter == termIdKeywordMap_.end() )
            {
                DLOG(ERROR) << "[PropertyTermInfo] Error Line(" << __LINE__ << ") : cannot found term id." << endl;
                return;
            }
            termStringList.push_back( termStringIter->second );
        }
    } // end - getPositionedTermStringList()

    void PropertyTermInfo::getPositionedFullTermString(izenelib::util::UString& termString) const
    {
        termString.clear();

        std::vector<izenelib::util::UString> termStringList;
        this->getPositionedTermStringList( termStringList );

        izenelib::util::UString SPACE_UCHAR(" ", izenelib::util::UString::UTF_8);
        std::vector<izenelib::util::UString>::iterator iter = termStringList.begin();
        for(; iter != termStringList.end(); iter++)
        {
            termString += *iter;
            if ( iter + 1 != termStringList.end() )
                termString += SPACE_UCHAR;
        }
    } // end - getPositionedFullTermString()


    void PropertyTermInfo::clear(void)
    {
        searchProperty_.clear();
        termIdPositionMap_.clear();
        termIdFrequencyMap_.clear();
        isWildCardQuery_ = false;
    }

} // namespace sf1r

