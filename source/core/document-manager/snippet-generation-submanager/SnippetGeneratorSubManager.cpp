/**
 * @file SnippetGeneratorSubManager.cpp
 * @author MyungHyun (Kent)
 * @date Created <2008-06-05>
 *
 * History:
 * - 2009-09-05 Deepesh Shrestha
 *   Modified interfaces to generate snippet of text and
 *   sentence offsets instead of word offsets for analyzed
 *   queryterms from index manager.
 * - 2009-11-13 Peiseng Wang
 *   getSnippet() modified to use dynamic programming for
 *   selecting the two highest score sentences
 * - 2009-11-20 Deepesh Shrestha
 *   computeInverseSentenceFrequency() for computing tf*ISF
 *   as a sentence metric for selecting snippet
 */

#include "SnippetGeneratorSubManager.h"
#include <cmath>
#include <util/profiler/ProfilerGroup.h>

using namespace std;
using namespace izenelib::util;
using namespace sf1r;

const unsigned int SnippetGeneratorSubManager::NUM_OF_SENTENCES = 2;
const unsigned int SnippetGeneratorSubManager::NUM_OF_PREFIX_WORD = 2;
const unsigned int SnippetGeneratorSubManager::MAX_TERM_FREQUENCY = 10;

///@brief search query terms in the logical block of texts and computes
///the weight based on term frequeny and relative distance between query terms
bool SnippetGeneratorSubManager::getSnippet(
    const UString &text,
    const vector<uint32_t>& offsetPairs,
    const std::vector<UString>& queryTerms,
    const unsigned int maxSnippetLength, 
    const bool bHighlight,
    izenelib::util::UString::EncodingType& encodingType, 
    UString& snippetText
)
{
    CREATE_SCOPED_PROFILER ( getSnippet, "SnippetGeneratorSubManager", "SnippetGeneratorSubManager::getSnippet");
    if (text.empty())
        return false;

    //ignore values in first few cells containing summaryoffset
    unsigned int i = offsetPairs[0]*2 + 1;

    double max_score1 = 0.0;
    double max_score2 = 0.0;
    int max_idx1 = -1;
    int max_idx2 = -1;
    int start_off1 = 0;
    int start_off2 = 0;
    int sen_off1 = 0;
    int sen_off2 = 0;

    std::vector<uint32_t> contained_term1;
    std::vector<uint32_t> contained_term2;

    for (; i < offsetPairs.size()-1; i+=2)
    {
        CharacterOffset start = offsetPairs[i+1];
        CharacterOffset end = offsetPairs[i];
        CharacterOffset start_off = offsetPairs[i+1];
        unsigned int tf = 0;
        double score = 0.0;

        UString subText;
        CharacterOffset length;

        if (offsetPairs[i+1] <= text.length())
            text.substr(subText, offsetPairs[i], offsetPairs[i+1]
                        - offsetPairs[i]);
        else
            continue;

        if (subText.empty())
            continue;

        length = subText.length();

        std::vector<uint32_t> contained_term;
        uint32_t termIndex=0;
        bool has_first_term = false;

        for (vector<UString>::const_iterator it = queryTerms.begin(); it
                != queryTerms.end(); it++, termIndex++)
        {
            CharacterOffset offset = 0;
            CharacterOffset s=-1, e=-1;
            bool first = true;
            //fetches character offset of query term string in text
            while ( ( (offset = subText.find(*it, offset, SM_IGNORE))
                      != izenelib::util::UString::NOT_FOUND ) && (offset < length)
                    && (tf < MAX_TERM_FREQUENCY))
            {
                if (termIndex == 0)
                {
                    has_first_term = true;
                }
                string str;
                if (contained_term.size()==0||contained_term[contained_term.size()-1]!=termIndex)
                    contained_term.push_back(termIndex);

                tf += 1;
                if (first)
                {
                    s = offset+ offsetPairs[i];
                    first = false;

                }
                e = offset + offsetPairs[i];
                offset += static_cast<CharacterOffset>(it->length());
            } //end while
            if ( s != (CharacterOffset)-1 && (start == 0 || s < start) )
                start = s;
            if ( start < start_off )
                start_off = start;
            if ( e > end )
                end = e;
        }
        if (queryTerms.size()>1&&contained_term1.size()==1&&contained_term.size()==1
                &&contained_term[0]==contained_term1[0])
            continue;
        if (queryTerms.size()>1&&contained_term2.size()==1&&contained_term.size()==1
                &&contained_term[0]==contained_term2[0])
            continue;
        score = computeScore_(tf, start, end);
        //keep track of two maximum score sentences
        if (has_first_term && score > max_score1)
        {
            max_score2 = max_score1;
            max_idx2 = max_idx1;
            contained_term2.swap(contained_term1);
            max_score1 = score;
            max_idx1 = i;
            contained_term1.swap(contained_term);

            start_off2 = start_off1;
            start_off1 = start_off;
            sen_off2 = sen_off1;
            sen_off1 = offsetPairs[i];
        }
        else if (score > max_score2)
        {
            max_score2 = score;
            max_idx2 = i;
            start_off2 = start_off;
            sen_off2 = offsetPairs[i];
            contained_term2.swap(contained_term);
        }
    } // end for all senteces
    // get result
    if (max_score1 == 0.0)
    {
        if (maxSnippetLength < text.length())
        {
            text.substr(snippetText, 0, maxSnippetLength);
            return true;
        }
        else
            return false;
    }
    if (max_score1 > 0)
    {
        if ( text.isChineseChar(start_off1) )
            start_off1 = (start_off1-sen_off1 < (int)maxSnippetLength/2)? sen_off1+1:start_off1 - maxSnippetLength/2;
        if (getSnippetText_(text, queryTerms, start_off1, start_off1 < start_off2 ? start_off2 : text.length()-1, maxSnippetLength,
                            bHighlight, encodingType, snippetText) == false)
            return false;
    }
    if (max_score2 > 0)
    {
        if ( text.isChineseChar(start_off2) )
            start_off2 = (start_off2-sen_off2 < (int)maxSnippetLength/2)? sen_off2+1:start_off2-maxSnippetLength/2;
        if (getSnippetText_(text, queryTerms, start_off2, start_off1 < start_off2 ? text.length()-1 : start_off1 , maxSnippetLength,
                            bHighlight, encodingType, snippetText) == false)
            return false;
    }
    return true;
}

///@brief generateSnippetText_() for cropping snippet text from sentence
bool SnippetGeneratorSubManager::getSnippetText_(
    const UString& text,
    const std::vector<UString>& queryTerms, 
    int snippetStarts,
    int nextSnippetStarts,
    const unsigned int maxSnippetLength, 
    const bool bHighlight,
    izenelib::util::UString::EncodingType& encodingType, 
    UString& snippetText
)
{
    if (text.empty())
        return false;
    CharacterOffset snippetStart = static_cast<CharacterOffset>(snippetStarts);
    CharacterOffset nSpace = 0;
    while (snippetStart> 0)
    {
        if ( text.isSpaceChar(snippetStart) ||text.isChineseChar(snippetStart) )
        {
            if (++nSpace > NUM_OF_PREFIX_WORD)
            {
                snippetStart++;
                break;
            }
        }
        snippetStart--;
    }

    CharacterOffset nextSnippetStart = static_cast<CharacterOffset>(nextSnippetStarts);
    if (nextSnippetStart != text.length()-1)
    {
        while (nextSnippetStart> 0)
        {
            if ( text.isSpaceChar(nextSnippetStart) ||text.isChineseChar(nextSnippetStart) )
            {
                if (++nSpace > NUM_OF_PREFIX_WORD)
                {
                    nextSnippetStart++;
                    break;
                }
            }
            nextSnippetStart--;
        }
    }

    izenelib::util::UString ellipse("...", encodingType);
    UString textFragment;

    if ( (maxSnippetLength/2) < text.length())
    {
        if ((nextSnippetStart - snippetStart)<(maxSnippetLength/2))
            text.substr(textFragment, snippetStart, nextSnippetStart - snippetStart);
        else
            text.substr(textFragment, snippetStart, (maxSnippetLength)/2);
    }
    else
        textFragment = text;

    if (bHighlight)
    {
        UString highlightedText;
        highlighter_.getHighlightedText(textFragment, queryTerms, encodingType,
                                        highlightedText);
        snippetText += ellipse;

        if (highlightedText.size() < 1)
            snippetText += textFragment;
        else
            snippetText += highlightedText;

    }
    else
    {
        snippetText += ellipse;
        snippetText += textFragment;
    }
    return true;
}

///@computes inverse sentence frequency and returns score
double SnippetGeneratorSubManager::computeInverseSentenceFrequency_(
    const vector<uint32_t>& offsetPairs,
    const vector<CharacterOffset>& termOffsets
)
{
    uint32_t i = offsetPairs[0]*2 + 1; //intialize to beginning of first sentence offset
    uint32_t k = 0;
    uint32_t ntsf = 0; //number of sentences with terms

    while ( (i <= offsetPairs.size() ) && (k < termOffsets.size() ))
    {
        if (termOffsets[k] < offsetPairs[i+1]) //check presence of terms within sentence boundry
        {
            ntsf+=1;
            k++;
        }
        i+=2;
    }

    //computes inverse sentence frequency
    if (ntsf > 0)
        return (log2( (offsetPairs.size() - offsetPairs[0]*2)/2) - log2(ntsf)
                +1);
    else
        return 0.0;
}

double SnippetGeneratorSubManager::computeScore_(
    const unsigned int tf,
    const unsigned int start, 
    const unsigned int end
)
{
    double K = 0.99;
    double score = 0.0;
    //compute score for snippet based on TF
    if (tf > 1 )
    {
        assert( end >= start);
        //*1000,to avoid float comparision
        score = K*tf /((1-K)*((end-start)/tf-1) );
    }
    else
    {
        score = K*tf;
    }

    return score;
}

