/*
 * EkQueryCorrection.cpp
 *
 *  Created on: Nov 3, 2010
 *      Author: wps
 */

/**@file   QueryCorrectionSubmanager.cpp
 * @brief  source file of Query Correction
 * @author Jinglei Zhao&Jinli Liu
 * @date   2009-08-21
 * @details
 * - Log
 *     - 2009.09.26 add new candidate generation
 *     - 2009.10.08 add new error model for english
 *     - 2009.11.27 add isEnglishWord() and isKoreanWord() to check if one word is mixure
 *      -2010.03.05 add log manager periodical worker.
 */

#include  "Util.h"
#include  "EkQueryCorrection.h"
#include  "MinuteEditDistance.h"
#include  "ViterbiNode.h"
#include  <list>
#include  <common/SFLogger.h>
#include  <fstream>
namespace {

static const float SMOOTH = 0.000001;
static const float FACTOR = 0.05;
static const int UNIGRAM_FREQ_THRESHOLD = 3;
static const int BIGRAM_FREQ_THRESHOLD = 3;
static const int UNIGRAM_FACTOR = 10;
static const int BIGRAM_FACTOR = 1;
static const unsigned int CHINESE_CORRECTION_LENGTH = 8;

}

using namespace izenelib::util::ustring_tool;
namespace sf1r {

EkQueryCorrection::EkQueryCorrection(const string& path,
		const std::string& workingPath, int ed) :
	path_(path), workingPath_(workingPath),activate_(true), max_ed_(ed), s_(NULL), s_k_(NULL),
			dictEN_(path_ + "/dictionary_english"), dictKR_(path_
					+ "/dictionary_korean"), dictENHash_(), dictKRHash_() {
	initialize();
}

void EkQueryCorrection::initDictHash(izenelib::am::rde_hash<
		izenelib::util::UString, bool>& hashdb, const std::string& fileName) {

	ifstream input(fileName.c_str());
	if (!input.is_open()) {
		DLOG(ERROR) << "Error : File(" << fileName << ") is not opened."
				<< endl;
		return;
	}
	std::string w;
	while (input.good()) {
		getline(input, w);
		if (w.find("\r") != std::string::npos)
			w = w.substr(0, w.length() - 1);
		izenelib::util::UString srcWord(w, izenelib::util::UString::UTF_8);
		srcWord.toLowerString();
		hashdb.insert(srcWord, true);
	}

}

bool EkQueryCorrection::inDict_(const izenelib::util::UString& ustr) {
	//izenelib::am::NullType nul;
	bool b1 = false;
	bool b2 = false;
	string str;
	ustr.convertString(str, UString::UTF_8);
	return (dictENHash_.get(ustr, b1) || dictKRHash_.get(ustr, b2));
}

void EkQueryCorrection::constructDAutoMata(const std::string& path) {
	matrix_ = path + "/Matrix.txt";
	DAutomata autoMataD;

	s_ = autoMataD.buildTrie(dictEN_, letters_, alphabet_, lexicon_);
	if (!s_) {
		DLOG(WARNING)
				<< "[Waring]: english dictionary automata build failed. Please ensure that you set the path SpellerSupport correctly in the configuration file."
				<< " And ensure the file dictionary_english is putted there."
				<< std::endl;
		activate_ = false;
		return;
	}
	s_k_ = autoMataD.buildTrie(dictKR_, jomas_, jomaMap_, lexicon_);
	if (!s_k_) {
		DLOG(WARNING)
				<< "[Waring]: korean dictionary automata build failed. Please ensure that you set the path SpellerSupport correctly in the configuration file."
				<< " And ensure the file dictionary_korean is putted there."
				<< std::endl;
		activate_ = false;
		return;
	}
	model_.assignMatrix(matrix_.c_str());

	UAutomata autoMataU(max_ed_);
	autoMataU.buildAutomaton(states_, autoMaton_);

}

//Initialize some member variables
bool EkQueryCorrection::initialize() {
	cogram_.reset(new
	//izenelib::am::sdb_fixedhash<
			izenelib::sdb::unordered_sdb_fixed<unsigned int, float,
					izenelib::util::ReadWriteLock>(path_ + "/co_gram"));

	if (!cogram_->open()) {
		std::string
				msg =
						"Cogram load failed. Please ensure that you set the path SpellerSupport correctly in the configuration file.";
		sflog->error(SFL_MINE, msg.c_str());
		DLOG(WARNING) << msg << std::endl;
		activate_ = false;
		return false;
	}
	initDictHash(dictENHash_, dictEN_);
	initDictHash(dictKRHash_, dictKR_);
	constructDAutoMata( path_);

	return true;
}

EkQueryCorrection::~EkQueryCorrection() {
	if (cogram_)
		cogram_->close();
	if (s_ != NULL)
		delete s_;
	if (s_k_ != NULL)
		delete s_k_;

}

bool EkQueryCorrection::getRefinedQuery(const std::string& collectionName,
		const UString& queryUString, UString& refinedQueryUString) {
	std::vector<UString> queryTokens;
	Tokenize(queryUString, queryTokens);
	bool needCorrect = false;
	bool queryFlag = true;
	std::vector<std::vector<CandidateScoreItem> > queryCandidates;
	vector<UString>::iterator it_token = queryTokens.begin();
	for (; it_token != queryTokens.end(); it_token++) {
		vector<UString> tokenUStrCandidates;
		(*it_token).toLowerString();

		//token exists in dict
		if (inDict_(*it_token))
			tokenUStrCandidates.push_back(*it_token);
		else {
			if ( isKoreanWord(*it_token) ) {
				queryFlag = false;
				candidateGeneration(*it_token, s_k_, tokenUStrCandidates, false);
				needCorrect = true;
			} else if( isEnglishWord(*it_token) )
			{
				queryFlag = true;
				candidateGeneration(*it_token, s_, tokenUStrCandidates, true);
				needCorrect = true;
			}
		}
		if (tokenUStrCandidates.size() == 0)//no candidates
		{
			refinedQueryUString.assign("", UString::UTF_8);
			return true;
		}
		std::vector<CandidateScoreItem> rankedCandidates;
		candidateRanking(*it_token, tokenUStrCandidates, rankedCandidates,
				queryFlag);//rank candidates
		queryCandidates.push_back(rankedCandidates);
	}

	//std::string strRefQuery = "";
	if (needCorrect) {
		std::vector<UString> refinedTokens;
		//for multiple query tokens, using Viterbi algorithm to find the best path
		bool ret = getBestPath_(collectionName, queryTokens, queryCandidates, refinedTokens);
		if (ret == false)
			return false;
		for (unsigned int i = 0; i < refinedTokens.size(); i++) {
			if (i + 1 < refinedTokens.size()){
				refinedQueryUString += refinedTokens[i] ;
			    refinedQueryUString += UString(" ", UString::UTF_8);
			}
			else
				refinedQueryUString += refinedTokens[i];
		}
	}
	//refinedQueryUString.assign(strRefQuery, UString::UTF_8);
	return true;

}

//Rank the candidates according to the error model and minute distance
bool EkQueryCorrection::candidateRanking(const UString& queryToken,
		const std::vector<UString>& queryCandidates, std::vector<
				CandidateScoreItem>& scoredCandidates, bool queryFlag) {

	// 	std::string candidate;
	if (queryFlag)// candidate is english
	{
		for (unsigned int i = 0; i < queryCandidates.size(); i++) {
			// 			queryCandidates[i].convertString(candidate, UString::UTF_8);
			float score = model_.getErrorModel(queryToken, queryCandidates[i]);
			CandidateScoreItem oneItem(queryCandidates[i], score);
			scoredCandidates.push_back(oneItem);
		}
	}
	else //candidate is korean,must decompose first
	{
		UString queryTokenDecomposer;
		processKoreanDecomposer(queryToken, queryTokenDecomposer);

		for (unsigned int i = 0; i < queryCandidates.size(); i++) {
			// 			queryCandidates[i].convertString(candidate, UString::UTF_8);
			UString queryCandidateDecomposer;
			processKoreanDecomposer(queryCandidates[i],
					queryCandidateDecomposer);

			float score = getErrorScore(queryTokenDecomposer,
					queryCandidateDecomposer);
			CandidateScoreItem oneItem(queryCandidates[i], score);
			scoredCandidates.push_back(oneItem);
		}
	}
	sort(scoredCandidates.begin(), scoredCandidates.end());

	return true;
}

//This error model is based on Minute EditDistance.
float EkQueryCorrection::getErrorScore(const UString& queryToken,
		const UString& queryCandidate) {

	std::string candidate;
	std::string strToken;
	queryToken.convertString(strToken, UString::UTF_8);
	queryCandidate.convertString(candidate, UString::UTF_8);
	float edit = MinuteEditDistance::getDistance(strToken.c_str(),
			candidate.c_str());
	return pow(SMOOTH, edit);

}

//Given query and character set, we can get the corresponding bitVector.eg: $$word|w=001000
void EkQueryCorrection::generateBitVector(const UString& queryToken, vector<
		mychar>& characters, vector<std::string> &bitVector) {
	string tempStr(max_ed_, '$');
	UString ustrWord(tempStr, UString::UTF_8);
	ustrWord += queryToken;

	std::string inputVector(ustrWord.length(), '0');
	for (unsigned int i = 0; i < characters.size(); i++) {
		for (unsigned int j = 0; j < ustrWord.length(); j++) {
			if (characters[i] == ustrWord[j])
				inputVector[j] = '1';
			else
				inputVector[j] = '0';
		}
		bitVector[i] = inputVector;
	}

}

//Generate candidate according to universal automata and dictionary automata.
//The process  is similar to DFS.
void EkQueryCorrection::candidateGeneration(const UString& queryUString,
		DictState *s, std::vector<UString>& queryCandidates, bool queryFlag) {

	vector<std::string> bitVector(letters_.size());
	vector<std::string> bitVectorKR(jomas_.size());
	UString tarUString;
	int len;
	if (queryFlag)//english
	{
		generateBitVector(queryUString, letters_, bitVector);
		len = queryUString.length();
	} else//korean need to decompose first
	{
		processKoreanDecomposer(queryUString, tarUString);
		generateBitVector(tarUString, jomas_, bitVectorKR);
		len = tarUString.length();
	}
	UString Ublank("", UString::UTF_8);
	CandidateGenerateItem item(0, Ublank, s, 0);
	std::stack<CandidateGenerateItem> Stack;
	Stack.push(item);
	CandidateGenerateItem nextItem;
	string inputVector;

	while (!Stack.empty()) {
		item = Stack.top();
		Stack.pop();
		//for every dictionary state q_d,transfer all of its outgoing edge.
		for (int i = 0; i < (item.q_d)->getOutCount(); i++) {

			transition* tran = &(item.q_d)->getTransitions()[i];
			//##############getInputVector begin###################//
			mychar ch = tran->getLabel();
			int bound = item.pos + max_ed_ + 1;
			int width = -item.pos + max_ed_ + 1;
			if (queryFlag)//english
			{
				int r = len <= bound ? len : bound;
				int chNo = alphabet_.find(ch)->second;
				inputVector = bitVector[chNo].substr(item.pos, r + width);
			} else//korean
			{
				int r = len <= bound ? len : bound;
				int chNo = jomaMap_.find(ch)->second;
				inputVector = bitVectorKR[chNo].substr(item.pos, r + width);

			}
			//###############getInputvector end #################//
			if (inputVector.empty())
				continue;

			rde_tran tempTrans(item.q_u, inputVector);
			//according to temp transition ,find next item in universal automaton
			rde::hash_map<rde_tran, int>::iterator it = autoMaton_.find(
					tempTrans);
			if (it != autoMaton_.end())
				nextItem.q_u = it->second;//next q_u
			else
				continue;
			nextItem.q_d = tran->getTarget();// next q_d
			if (nextItem.q_d) {
				nextItem.labels = item.labels;
				nextItem.labels += ch; // next labels
				nextItem.pos = item.pos + 1; // next position
				Stack.push(nextItem);
				bool isDFinal = nextItem.q_d->isFinal();
				bool isUFinal = states_[nextItem.q_u][0].isFinal();
				if (isDFinal && isUFinal) {
					UString candidate;
					processKoreanComposer(nextItem.labels, candidate);
					queryCandidates.push_back(candidate);
				}
			}
		}
	}
}

float EkQueryCorrection::transProb(const std::string& w1, const std::string& w2) {
	string word;
	word.append(w1);
	word.append(" ");
	word.append(w2);
	float prob = SMOOTH;
	float solo_num, co_num;

	if (cogram_->get(hashFunc(w1), solo_num) && cogram_->get(hashFunc(word),
			co_num)) {
		prob = 1.0 * (SMOOTH + co_num) / (solo_num + SMOOTH);
		//cout<<"w1 "<<w1<<"->"<<solo_num<<"\t ("<<w1<<","<<w2<<")->"<<co_num<<endl;
	}
	return prob;
}

bool EkQueryCorrection::Tokenize(const UString& queryUString, std::vector<
		UString>& queryTokens) {
	char delimiter = ' ';
	getTokensFromUString(UString::UTF_8, delimiter, queryUString, queryTokens);
	return true;
}

unsigned int EkQueryCorrection::hashFunc(const std::string &str) {
	unsigned int v = 0;

	for (int i = 0; i < (int) str.length(); i++) {
		v = 37 * v + str[i];
	}
	return v;
}

void EkQueryCorrection::updateCogramAndDict(const std::list<std::pair<
		izenelib::util::UString, uint32_t> >& recentQueryList) {
	updateCogramAndDict("", recentQueryList);
}

void EkQueryCorrection::updateCogramAndDict(
		const std::string& collectionName,
		const std::list<std::pair<izenelib::util::UString, uint32_t> >& recentQueryList) {
	boost::mutex::scoped_lock scopedLock(logMutex_);

	DLOG(INFO) << "updateCogramAndDict..." << endl;
	const std::list<std::pair<izenelib::util::UString, uint32_t> >& queryList =
			recentQueryList;
	std::list<std::pair<izenelib::util::UString, uint32_t> >::const_iterator
			lit;

	//  LogManager::getRecentQueryList(queryList);
	//update Chinese Ngram


	//update English and Korean
	//     bool isReconstruct = false;
	//     izenelib::am::NullType nul;
	//     std::vector<izenelib::util::UString> querywords;
	//     std::vector<izenelib::util::UString>::iterator wit;
	//
	//     //What about korean?
	//     for (lit=queryList.begin(); lit != queryList.end(); lit++) {
	//         //cout<<lit->first<<"  "<<lit->second<<endl;
	//         Tokenize(lit->first, querywords);
	//
	//         //assert(querywords.size() >=1);
	//         if (querywords.size() < 1)
	//             continue;
	//         string last_str = "";
	//         for (wit=querywords.begin(); wit != querywords.end(); wit++) {
	//             uint32_t freq = lit->second;
	//             if ((int)freq > UNIGRAM_FREQ_THRESHOLD) {
	//                 if (isKoreanWord(*wit) ) {
	//                     if (dictKRSdb_.insert(*wit, nul) ) {
	//                         isReconstruct = true;
	//                         dictKRSdb_.commit();
	//                     }
	//                 } else  if ( !hasChineseChar(*wit) )
	//                 {
	//                     if (dictENSdb_.insert(*wit, nul) ) {
	//                         isReconstruct = true;
	//                         dictENSdb_.commit();
	//                     }
	//                 }
	//             }
	//
	//             //to be fixed,updation of cogram
	//          float num=0.0;
	//          string word;
	//          wit->convertString(word, UString::UTF_8);
	//
	//          //cout<<"update "<<word<<endl;
	//          if (inDict_( *wit) ) {
	//              unsigned int key = hashFunc(word);
	//              //if( cogram_->get(key, num) )
	//              {
	//                  DLOG(INFO)<<"update: word="<<word<<", increase freq="
	//                          <<freq<<endl;
	//                  cogram_->update(key, num+freq*UNIGRAM_FACTOR);
	//              }
	//          }
	//
	//          //update bigram
	//          if (last_str != "") {
	//              string bigram = last_str +" " + word;
	//              UString ubigram = izenelib::util::UString(bigram, UString::UTF_8);
	//              int bfreq = lit->second
	//              unsigned int key = hashFunc(bigram);
	//              float num = 0.0;
	//              if (cogram_->get(key, num) || (inDict_(izenelib::util::UString(
	//                      last_str, UString::UTF_8) ) && inDict_(*wit) && bfreq
	//                      > BIGRAM_FREQ_THRESHOLD )) {
	//                  DLOG(INFO)<<"update: bigram="<<last_str<<" + "<<word
	//                          <<", increase  freq=" <<num<<endl;
	//                  cogram_ -> update(key, num + bfreq*BIGRAM_FACTOR);
	//              }
	//          }
	//          last_str = word;
	//         }
	//
	//     }
	//     cogram_->commit();
	//     if (isReconstruct) {
	//         DLOG(INFO)<<" Reconstructing DAutoMata..."<<endl;
	//         //dictENSdb_->display();
	//         constructDAutoMata(path_);
	//     }
}

float EkQueryCorrection::transProb(const izenelib::util::UString& w1,
		const izenelib::util::UString& w2) {
	std::string s1, s2;
	w1.convertString(s1, izenelib::util::UString::UTF_8);
	w2.convertString(s2, izenelib::util::UString::UTF_8);
	return transProb(s1, s2);
}

unsigned int EkQueryCorrection::hashFunc(const izenelib::util::UString& ustr) {
	std::string str;
	ustr.convertString(str, izenelib::util::UString::UTF_8);
	return hashFunc(str);
}

bool EkQueryCorrection::getCandidateItems_(const std::string& collectionName,
		const std::vector<izenelib::util::UString>& tokens, std::vector<std::vector<
				CandidateScoreItem> >& candidateItems) {
	bool needCorrect = false;
	candidateItems.resize(tokens.size());
	for (uint32_t i = 0; i < tokens.size(); i++) {
		izenelib::util::UString token = tokens[i];

		bool queryFlag = true;

		std::vector<izenelib::util::UString> tokenUStrCandidates;
		if (inDict_(token))//token exists in dict
		{
			tokenUStrCandidates.push_back(token);
		} else {
			if (isKoreanWord(token)) {
				queryFlag = false;
				candidateGeneration(token, s_k_, tokenUStrCandidates,
						false);
				needCorrect = true;
			} else {
				queryFlag = true;
				candidateGeneration(token, s_, tokenUStrCandidates, true);
				needCorrect = true;
			}
		}
		candidateItems[i].resize(0);
		if (tokenUStrCandidates.size() > 0) {
			for (size_t j = 0; j < tokenUStrCandidates.size(); j++) {
				std::string str;
				tokenUStrCandidates[j].convertString(str,
						izenelib::util::UString::UTF_8);
				//std::cout<<"!!!!common tokenUStrCandidates "<<str<<std::endl;
			}
			candidateRanking(token, tokenUStrCandidates,
					candidateItems[i], queryFlag);//rank candidates
		}

	}
	return needCorrect;
}



bool EkQueryCorrection::getBestPath_(const std::string& collectionName, const std::vector<UString>& queryTokens,
		const std::vector<std::vector<CandidateScoreItem> >& queryCandidates,
		std::vector<UString>& refinedCandidate) {
	float t_prob;
	int size = queryCandidates[0].size();
	std::vector<ViterbiNode> T, U;
	T.resize(size);
	//deal with the first token's candidates
	for (int i = 0; i < size; i++) {
		if (cogram_->get(hashFunc(queryCandidates[0][i].path_), t_prob)) {
			T[i].prob = (t_prob + FACTOR) * queryCandidates[0][i].score_;
			//cout<<queryCandidates[0][i].candidate_<<" -> "<<endl;
		} else
			T[i].prob = FACTOR * queryCandidates[0][i].score_;

		T[i].v_path.push_back(i);
		T[i].v_prob = T[i].prob;

		//      cout<<"\n!!!!!!"<<t_prob<<"  "<<queryCandidates[0][i].candidate_
		//              <<" -> "<<queryCandidates[0][i].score_<<" -> "<<T[i].prob<<endl;

	}

	for (int i = 0; i < (int) queryTokens.size() - 1; i++) {
		int size = queryCandidates[i + 1].size();
		if (size <= 0)
			return false;
		else
			U.resize(size);
		//find local best path from i+1 token to end
		for (int j = 0; j < size; j++) {
			float total = 0.0;
			std::vector<unsigned int> argmax;
			float valmax = 0.0;
			for (unsigned int k = 0; k < queryCandidates[i].size(); k++) {
				ViterbiNode node;
				node = T[k];
				float freq = 1.0;
				cogram_->get(hashFunc(queryCandidates[i + 1][j].path_), freq);
				float p = queryCandidates[i + 1][j].score_ * freq;
				p *= transProb(queryCandidates[i][k].path_, queryCandidates[i
						+ 1][j].path_);
				node.prob *= p;
				node.v_prob *= p;
				total += node.prob;
				if (node.v_prob > valmax) {
					valmax = node.v_prob;
					argmax = node.v_path;
					argmax.push_back(j);
				}
			}
			U[j].prob = total;
			U[j].v_path = argmax;
			U[j].v_prob = valmax;
		}
		T = U;
	}
	float total = 0.0;
	std::vector<unsigned int> argmax;
	float valmax = 0.0;
	for (unsigned int i = 0; i
			< queryCandidates[queryCandidates.size() - 1].size(); i++) {
		total += T[i].prob;
		if (T[i].v_prob >= valmax) {
			valmax = T[i].v_prob;
			argmax = T[i].v_path;
		}
	}

	std::vector<izenelib::util::UString> refinedTokens;

	// Add condition of segmentation fault occuring error.
	if ((queryTokens.size() > queryCandidates.size()) || (queryTokens.size()
			> argmax.size()))
		return false;

	for (unsigned int i = 0; i < queryTokens.size(); i++)
		refinedTokens.push_back(queryCandidates[i][argmax[i]].path_);

	izenelib::util::UString strRefQuery;
	for (unsigned int i = 0; i < refinedTokens.size(); i++) {
		strRefQuery += refinedTokens[i];
		if (i + 1 < refinedTokens.size()) {
			strRefQuery += izenelib::util::UString(" ",
					izenelib::util::UString::UTF_8);
		}
	}
	refinedCandidate.push_back( strRefQuery );
	return true;
}

}/*namespace sf1r*/

