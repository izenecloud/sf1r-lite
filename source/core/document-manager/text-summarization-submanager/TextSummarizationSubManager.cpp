/**
 * @file TextSummarizationSubManager.cpp
 * @brief header file of TextSummarizationSubManager class
 * @author MyungHyun (Kent)
 * @date 2008-06-05
 * @details
 * - Log
 * @author Deepesh Shrestha
 *   Modified interfaces to generate sentence blocks for
 *   snippet and summarization
 */

#include "TextSummarizationSubManager.h"
#include "text-summarization/TextSummarization.h"

using namespace std;
using namespace izenelib::util;     //for UString
using namespace sf1r::text_summarization;

namespace sf1r
{

TextSummarizationSubManager::TextSummarizationSubManager()
{
}

TextSummarizationSubManager::~TextSummarizationSubManager()
{
}

void TextSummarizationSubManager::init(
    ilplib::langid::Analyzer* langIdAnalyzer,
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager 
)
{
    langIdAnalyzer_ = langIdAnalyzer;
    idManager_ = idManager;
}

////@brief get requests for getting summary, snippet offsets, when given a text
bool TextSummarizationSubManager::getOffsetPairs(  
    const izenelib::util::UString& textBody,
    const LAInput& laInput,
    const unsigned int maxDisplayLength,
    const unsigned int numOfSentence,
    std::vector<CharacterOffset>& offsetPairs
)

{
    //clear out structure
    offsetPairs.clear();

    //The number of sentences in the text
    //vector<pair<CharacterOffset, UString> > sentenceText;
    //=== RUN a  sentence parser
    //SentenceParser parser;
    //parser.parseText2Sentences( textBody, maxDisplayLength, sentenceText );


    std::string utf8_text;
    textBody.convertString(utf8_text, izenelib::util::UString::UTF_8);
    const char* p = utf8_text.c_str();
//        std::string sentStr;

    std::vector<std::vector<TermId> > sentenceListInTermId;
    std::vector<std::pair<CharacterOffset, CharacterOffset> > sentencesOffsetPairs;

    CharacterOffset startPos = 0;
    while (int len = langIdAnalyzer_->sentenceLength(p))
    {
//            sentStr.assign(p, len); // get each sentence
//            UString sentence;
//            /// ineffecive here, we need such interface of UString:
//            ///assign(const char*, size_t len, EncodingType encodingType)
//            sentence.assign(sentStr, izenelib::util::UString::UTF_8);

        UString sentence;
        /// Add an efficient UString initializer for it. - Wei, 2010.08.25
        sentence.assign(p, len, izenelib::util::UString::UTF_8);

//            la::TermList termList;
//            tokenizer_.tokenize(sentence, termList);
//            TermId id;
//            sentenceListInTermId.push_back(vector<TermId>());
//            vector<TermId>& sentenceIds = sentenceListInTermId.back();
//            for(la::TermList::iterator termIter = termList.begin();
//                termIter != termList.end(); ++termIter)
//            {
//                idManager_->getTermIdByTermString(termIter->text_, id);
//                sentenceIds.push_back(id);
//            }

        /// Replace the old inefficient API to the new one. - Wei, 2010.08.25
        TermId id;
        sentenceListInTermId.push_back(vector<TermId>());
        vector<TermId>& sentenceIds = sentenceListInTermId.back();

        tokenizer_.tokenize(sentence);
        while (tokenizer_.nextToken())
        {
            if (!tokenizer_.isDelimiter())
            {
                idManager_->getTermIdByTermString(tokenizer_.getToken(), tokenizer_.getLength(), id);
                sentenceIds.push_back(id);
            }
        }

        sentencesOffsetPairs.push_back(make_pair(startPos, startPos+sentence.length()));
        startPos+=sentence.length();
        p += len;				 // move to the begining of next sentence
    }



    //sentencesOffsetPairs holds start, end offsets for snippet generation
    //std::vector<std::pair<CharacterOffset, CharacterOffset> > sentencesOffsetPairs;
    //getSentencesOffsetPairs( sentenceText, sentencesOffsetPairs );

    //initialize first value to be 0 by default
    offsetPairs.push_back(0);

    //append summary offset
    vector<unsigned int> sentenceOrder;
    if (numOfSentence > 0)
    {
        //sentenceListInTermId holds the termIds of the sentences
        //vector<vector<TermID> > sentenceListInTermId;
        //getSentenceListInTermId( sentenceWordLength, laInput, sentenceListInTermId);

        TextSummarization ts( numOfSentence );
        ts.computeSummaryRM(numOfSentence, sentenceListInTermId, sentenceOrder);

        //adding summary sentence Offset pair
        unsigned int i=0;
        while ( ( i < sentenceOrder.size()) && (i < numOfSentence))
        {
            if (sentenceOrder[i] < sentencesOffsetPairs.size())
            {
                offsetPairs.push_back(sentencesOffsetPairs[sentenceOrder[i]].first);
                offsetPairs.push_back(sentencesOffsetPairs[sentenceOrder[i]].second);
            }
            i++;
        }

        if (numOfSentence < sentenceOrder.size())
            offsetPairs[0] = numOfSentence;
        else
            offsetPairs[0] = sentenceOrder.size();


        sentenceOrder.clear();
    }

    //adding snippet sentence offset pair
    unsigned int j = 0;
    while ( j < sentencesOffsetPairs.size())
    {
        offsetPairs.push_back(sentencesOffsetPairs[j].first);
        offsetPairs.push_back(sentencesOffsetPairs[j].second);
        j++;
    }

    if (offsetPairs.size() > 0)
        return true;
    else
        return false;
}



//======================  PRIVATE METHODS  ======================

/// @brief retrieves the start, end offset pairs for snippet sentences
void TextSummarizationSubManager::getSentencesOffsetPairs(
    const std::vector<pair<CharacterOffset, UString> >& sentenceText,
    std::vector<std::pair<CharacterOffset, CharacterOffset> >& sentencesOffsetPairs
) const
{
    sentencesOffsetPairs.clear();

    for (vector<pair<CharacterOffset, UString> >::const_iterator iterSent = sentenceText.begin();
            iterSent != sentenceText.end(); ++iterSent)
    {
        sentencesOffsetPairs.push_back(pair<CharacterOffset, CharacterOffset>(
                                           iterSent->first, ( iterSent->first + (iterSent->second).length()) ) );
    }
}


/// @brief returns the sentence term ID lists for computing summary
void TextSummarizationSubManager::getSentenceListInTermId(
    const vector<WordOffset>&  sentenceWordLength,
    const LAInput& laInput,
    vector<vector<TermId> >& sentenceListInTermId
) const
{
    //3. Arrange the terms in sentences.
    //-------------------------------------------------------------------------------------
    sentenceListInTermId.reserve( sentenceWordLength.size() );
    vector<vector<TermId> > sentenceInTermId;

    //unsigned int lengthSum = 0;     //the sum of sentence lengths up to this point
    unsigned int sentenceIndex = 0;  //index for each sentence
    vector<TermId> sentenceVector;  //saves the termIDs of the words in the current sentence for temporary use

    //The sentences are  added to the text vector
    LAInput::const_iterator sorted_it;
    for ( sorted_it = laInput.begin()
                      ; (sorted_it != laInput.end())
            && (sentenceIndex < sentenceWordLength.size())
            ; sorted_it++ )
    {
        //if the current term's byte offset exceeds the current sentence boundary.
        if ( sorted_it->wordOffset_ >= sentenceWordLength[sentenceIndex] )
        {
            sentenceInTermId.push_back( sentenceVector );
            sentenceVector.clear();
            sentenceIndex++;
        }

        //saves the term to the sentence (a vector of terms)
        sentenceVector.push_back( sorted_it->termid_ );
    }
    sentenceListInTermId.swap(sentenceInTermId);

}//end getSentenceListInTermId

} //end namespace

