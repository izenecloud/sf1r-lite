#if !defined(_TEXT_SUMMARIZATION_SUBMANAGER_H_)
#define _TEXT_SUMMARIZATION_SUBMANAGER_H_

/**
 * @file TextSummarizationSubManager.h
 * @brief Defines the TextSummarizationSubManager class
 * @author MyungHyun (Kent)
 * @date 2008-06-05
 *  History:
 * - Deepesh Shrestha
 *   Modified interfaces to generate sentence blocks for
 *   snippet and summarization
 */

#include <ir/index_manager/index/LAInput.h>
#include <ir/id_manager/IDManager.h>

#include <langid/analyzer.h>

#include <util/ustring/UString.h>
#include <la/tokenizer/Tokenizer.h>
#include <la/util/UStringUtil.h>

#include <vector>
#include <map>

using namespace izenelib::ir::indexmanager;

namespace sf1r
{

    /**
     * @brief class of TextSummarizationSubManager returns summary and snippet
     * sentence offset pairs to the user. Uses Sentence parser to actually parse
     * the given text.
     */
    class TextSummarizationSubManager
    {
        protected:
            //---------------------------  TYPEDEFS  ---------------------------
            typedef unsigned int TermId;
            typedef uint32_t CharacterOffset;
            typedef uint32_t WordOffset;


        public:
            //---------------------------  PUBLIC METHODS  ---------------------------

            /**
             * @brief constructor
             */
            TextSummarizationSubManager();

            /**
             * @brief destructor
             */
            ~TextSummarizationSubManager();


            void init(ilplib::langid::Analyzer* langIdAnalyzer, boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager );

            /**
             * @brief get requests for getting summary, snippet offsets, when given a text
             *
             * @param textBody              The text subject to be summarized.
             * @param forwardIndex          The forward index data for the terms in the textbody
             * @param maxDisplayLength      Display length of sentence as defined in configuration
             * @param numOfSentence         Number of sentences to be produced as a part of summary
             *                              snippet offset block generator do not use this @param numOfSentence
             * @param offsetPairs           Ordered offset pair of summary and snippet sentences in one vector
             *                              First offsetPairs[0] contains the number of summary sentences
             * @return  Returns false if requested number of sentences were not created.
             */
            bool getOffsetPairs(
                    const izenelib::util::UString& textBody,
                    const LAInput& laInput,
                    const unsigned int maxDisplayLength,
                    const unsigned int numOfSentence,
                    std::vector<CharacterOffset>& offsetPairs
                    );


        private:
            //---------------------------  PRIVATE METHODS  ---------------------------

            /**
             * @brief Returns termIds of sentences in the order of sentence. Used for
             * summarization algorithm
             * @param sentenceWordLength    Holds number of words that each sentence contain
             * @param forwardIndex          Holds the offset information of each unique term
             *                              in the text. Produced by LAManager
             * @param sentenceListInTermId  The text represented as a sequence of sentences,
             *                              which are of a sequence of TermIDs
             */
            void getSentenceListInTermId(
                    const vector<WordOffset>&  sentenceWordLength,
                    const LAInput& laInput,
                    vector<vector<TermId> >& sentenceListInTermId
                    ) const;

            /**
             * @brief                       returns start, end pair of character offset of sentences.
             *                              summarization algorithm
             * @param sentenceText          holds pair of startOffset and sentence
             * @param sentenceOffsetPairs   start, end pair of sentences in sentenceText
             */
            void getSentencesOffsetPairs(
                    const std::vector<pair<CharacterOffset, izenelib::util::UString> >& sentenceText,
                    std::vector<std::pair<CharacterOffset, CharacterOffset> >& sentencesOffsetPairs
                    ) const;

        public:
            ilplib::langid::Analyzer* langIdAnalyzer_;
            ///We have to introduce tokenizer and idmanager here
            ///until charoffset could be got from LAManager
            boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager_;

            la::Tokenizer tokenizer_;
    };

}


#endif  //_TEXT_SUMMARIZATION_SUBMANAGER_H_
