/**
 * @file Highlighter.cpp
 * @brief header file of Highlighter
 * @author MyungHyun (Kent)
 * @date 2008-09-04
 *
 * History:
 *  -Deepesh 
 *  Interfaces to produce highlighting tags based
 *  on string matching on text since word offsets
 *  are no longer availed from document manager
 */

#include "Highlighter.h"

using namespace std;
using namespace izenelib::util;

const unsigned int Highlighter::START_FLAG = 0;
const unsigned int Highlighter::END_FLAG = 1;

///@brief getHighlightedText() Gets the text with highlighting tags. 
///uses string matching

bool Highlighter::getHighlightedText(
        const UString& textFragment,
        const vector<UString>& termStringList,
        const izenelib::util::UString::EncodingType& encodingType,
        UString& highlightedText
        ) const
{
    //stores the offsets of the start and end offsets of all terms
    vector<pair<CharacterOffset, OffsetFlag> > highlightTagPositions;

    //gets the start and end tag postion for the terms
    std::vector<UString>::const_iterator termIter =  termStringList.begin();;
    for(; termIter !=  termStringList.end(); ++termIter )
        getHighlightTagPositions( textFragment, *termIter, highlightTagPositions);

    if( highlightTagPositions.size() > 0 )
    {
        sort( highlightTagPositions.begin(), highlightTagPositions.end() );
        setHighlightTags(textFragment, 
                        highlightTagPositions, 
                        encodingType, 
                        highlightedText);
        return true;
    }
    return false;
}

///@brief returns higlightTagOffset list using string  matching approach
void Highlighter::getHighlightTagPositions( 
        const izenelib::util::UString& textFragment,
        const UString& termString,
        std::vector<std::pair<CharacterOffset, OffsetFlag> >& highlightTagPositions
        ) const
{
    CharacterOffset offset = 0;

    bool numTerm = false;
    bool isCJK= false;
    for (unsigned int i=0; i < termString.length(); i++)
    {
        if (termString.isChineseChar(i)||termString.isKoreanChar(i) || termString.isJapaneseChar(i))    
        {
            isCJK = true;
            break;
        }
        else if (termString.isNumericChar(i))    
        {
            numTerm = true;
        }
    }

    unsigned int termLength = termString.length();

    while( ( (offset = textFragment.find( termString, offset, SM_IGNORE ) ) 
            != izenelib::util::UString::NOT_FOUND) 
            && (offset+termLength <= textFragment.length() ) )
    {

        //rule for numeric terms 
        if ( numTerm )
        {
            if ((offset>0) && (offset+termLength < textFragment.length()))
            {
                if  ( textFragment.isNumericChar(offset-1) 
                        || textFragment.isNumericChar(offset+termLength) )   
                {
                    offset = termLength + offset;
                    continue;
                }
            }
            else {
                if (offset+termLength < textFragment.length())
                {
                    if  (textFragment.isNumericChar(offset+termLength))
                    {
                        offset = termLength + offset;
                        continue;
                    }
                }
            }
        }
        //rule out prefixes only if it is english terms
        else if (!isCJK)
        {
            if ((offset>0) && (offset+termLength < textFragment.length()))
            {
                if ((textFragment.charType(offset-1) == UCS2_ALPHA)
                        || (textFragment.charType(termLength+offset) == UCS2_ALPHA))
                {
                    offset = termLength + offset;
                    continue;
                }
            }
        }

        //set <!HS> start flag
        highlightTagPositions.push_back( 
                pair<CharacterOffset, OffsetFlag>(
                    offset, START_FLAG) 
                );


        offset = termLength + offset;
        // set <!HE> end flag
        highlightTagPositions.push_back( 
                pair<CharacterOffset, OffsetFlag>(
                    static_cast<CharacterOffset>(offset), END_FLAG) 
                );
        offset++;
    }
}

void Highlighter::setHighlightTags(
        const izenelib::util::UString& textFragment,
        const vector<pair<CharacterOffset, OffsetFlag> >& highlightTagPositions,
        const izenelib::util::UString::EncodingType& encodingType,
        UString& highlightedText
        ) const
{
    static UString startTag_("<!HS>", encodingType); 
    static UString endTag_("<!HE>", encodingType); 

    UString subStr;
    vector<pair<CharacterOffset, OffsetFlag> >::const_iterator it;

    //the start of the starting flag
    unsigned int startOffset = 0;

    //count the number of open START-END tags. A START tag that has no corresponding END tag is "open"
    unsigned int openFlagCount = 0;

    for( it = highlightTagPositions.begin(); 
            it != highlightTagPositions.end(); ++it )
    {
        if (it->first > textFragment.length())
        {
            continue;
        }
        if( it->second == START_FLAG )
        {
            openFlagCount++;
            if ( openFlagCount == 1 ) 
            {
                textFragment.substr( 
                        subStr, 
                        startOffset, 
                        it->first - startOffset 
                        );

                //add tags to highlighted textFragment
                highlightedText += subStr;
                highlightedText += startTag_;

                //set variables
                startOffset = it->first;
            }
        }
        else if( it->second == END_FLAG )
        {
            openFlagCount--;

            if( openFlagCount == 0 )
            {
                textFragment.substr( subStr, startOffset, it->first - startOffset );

                //add tags to highlighted textFragment
                highlightedText += subStr;
                highlightedText += endTag_;

                //set variables
                startOffset = it->first;
            }
        }
    } 

    //append remaining part of string
    if (startOffset < textFragment.length() )
    {
        textFragment.substr( 
                subStr, 
                startOffset, 
                textFragment.length() - startOffset 
                );

        highlightedText += subStr;
    }
}

