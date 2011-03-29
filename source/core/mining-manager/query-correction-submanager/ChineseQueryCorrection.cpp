#include "ChineseQueryCorrection.h"
#include "MinuteEditDistance.h"
#include <fstream>
#include <algorithm>
#include <queue>
#include <boost/filesystem.hpp>
#include <am/external_sort/izene_sort.hpp>
#include <common/SFLogger.h>
using namespace std;

namespace bfs = boost::filesystem;

namespace sf1r {

const int CQC_MAX_ONE_PINYIN_LENGTH = 6;
const unsigned int CQC_NBEST_QUERY_CORRECTION = 3;
const int CQC_WORD_FACTOR = 4;
const double CQC_COMMON_HANZI_FACTOR = 3.0;
const unsigned int CQC_UNIGRAM_FACTOR=100;
const unsigned int CQC_BIGRAM_FACTOR=5;
const unsigned int CQC_TRIGRAM_FACTOR=1;
const double CQC_DICTIONARY_FACTOR=1.1;

string toLower(const string& str) {
	string szResp(str);
	std::transform(szResp.begin(), szResp.end(), szResp.begin(),
			(int(*)(int))std::tolower);
	return szResp;
}

bool compareCandidateScore(const CandidateScorePtr& p1,
		const CandidateScorePtr& p2) {
	return p1->score> p2->score;
}

ChineseQueryCorrection::ChineseQueryCorrection(const string& path, const std::string& workingPath) :
	path_(path), workingPath_(workingPath), isOpen_(false),
    pinyin2phrase_(), phrase2pinyin_(), ngramFreq_(path +"/chn_corpus_ngram"),
			wordLogFreq_(path +"/chn_log_ngram") {
	

}

ChineseQueryCorrection::~ChineseQueryCorrection()
{
    if( isOpen_ )
    {
        ngramFreq_.close();
        wordLogFreq_.close();
    }
}

void ChineseQueryCorrection::warmUp(){
	ngramFreq_.fillCache();
	wordLogFreq_.fillCache();
}


void ChineseQueryCorrection::initialize() {
    std::string pinyinDict = path_ + "/pinyin.txt";
    //std::string phraseDict = path_ + "/chn_pinyin_dict.txt";
    std::string phraseDict = path_ + "/chn_pinyin_dict.txt";

    ngramFreq_.open();
    if (!wordLogFreq_.open()) {
        std::string msg = "Query correction log sdb open failed. Please ensure that you set the working path SpellerSupport correctly in the configuration file.";
        sflog->error(SFL_MINE, msg.c_str());
        DLOG(WARNING)<<msg<<std::endl;
    }
    isOpen_ = true;
	ifstream pinyinInput(pinyinDict.c_str() );
	ifstream phraseInput(phraseDict.c_str() );
	string line;
    //冠 guan1
    while (pinyinInput.good() ) {
		string chinese;
		string pinyin;

		pinyinInput>>chinese;
		pinyinInput>>pinyin;
		pinyin = pinyin.substr(0, pinyin.length()-1);
		vector<string> pinyins = pinyinMap_[chinese];
		vector<string> chineses = chineseMap_[pinyin];

		if (std::find(pinyins.begin(), pinyins.end(), pinyin) == pinyins.end() ) {
			pinyins.push_back(pinyin);
			pinyinMap_[chinese] = pinyins;
		}
		if (std::find(chineses.begin(), chineses.end(), chinese)
				== chineses.end() ) {
			chineses.push_back(chinese);
			chineseMap_[pinyin] = chineses;
		}
		//don't store pinyin for single chinese
        std::vector<std::string> chs;
        pinyin2phrase_.get(pinyin, chs);
        if (std::find(chs.begin(), chs.end(), chinese) == chs.end() ) {
          chs.push_back(chinese);
          pinyin2phrase_.update(pinyin, chs);
   		}
	}
#ifdef DEBUGDEBUG
    for(DictType::iterator ch_it = chineseMap_.begin();ch_it!=chineseMap_.end(); ch_it++){
    	cout << (ch_it->first) << " => ";
    	for(int i=0;i<ch_it->second.size();i++)
    		cout<< (ch_it->second).at(i)<<" ";
    	cout<<endl;

    }
#endif


	//xiashui  下水
	while (phraseInput.good() ) {
		std::string phrase;
		std::string pinyin;
		phraseInput>>pinyin;
		phraseInput>>phrase;		

		std::vector<std::string> chineses;
		izenelib::util::UString ustr(phrase,izenelib::util::UString::UTF_8);
		if (ustr.length()>0 && ustr.isChineseChar(0) )
			phrase2pinyin_.insert(phrase, 0);
		pinyin2phrase_.get(pinyin, chineses);	
		if (std::find(chineses.begin(), chineses.end(), phrase)
				== chineses.end() ) {
			chineses.push_back(phrase);			
			pinyin2phrase_.del(pinyin);
			pinyin2phrase_.insert(pinyin, chineses);
		}	
		updatePinyinStat_(pinyin, ustr);
	}
}

void ChineseQueryCorrection::updatePinyinStat_(const string& pinyin,
		const izenelib::util::UString &ustr) {
	vector<vector<string> > pinyinSegs;
	if (pinyinSegment_(pinyin, pinyinSegs) ) {
		for (size_t i=0; i<pinyinSegs.size(); i++) {
			if (pinyinSegs[i].size() == ustr.length() ) {
				for (size_t j=0; j<pinyinSegs[i].size(); j++) {
					++pinyinStat_[pinyinSegs[i][j] ];
					if (ustr.isChineseChar(j) ) {
						string ch;
						ustr.substr(j,1).convertString(ch, izenelib::util::UString::UTF_8);
						++pinyinHanziStat_[make_pair(pinyinSegs[i][j], ch) ];
					}
				}

			}
		}
	}
}

void ChineseQueryCorrection::train(const std::string& corpusDir) {
	//for test
	//
	//	std::vector<std::string> test;
	//	getRefinedQuery("ruanjiangongsi", test);
	//	getRefinedQuery("软jian", test);
	//	getRefinedQuery("nishishui", test);
	//	getRefinedQuery("woyaochifanheshuijiao", test);
	//	std::vector<std::string> pinyin;
	//	getPinyin("银行", pinyin);
	//	getPinyin("上海交通大学", pinyin);
	//	getPinyin("参差不齐", pinyin);
	//	getPinyin("台湾向前走", pinyin);

	//	assert(isPinyin("nidexiaorongwokanbudong") ==true );
	//	assert(isPinyin("nidexiaorongwolkanbudong") ==false );
	//	assert(isPinyin("ruanjiangongsi") ==true );
	//	assert(isPinyin("nishishui")==true );
	//	assert(isPinyin("xian")==true );
	//	assert(isPinyin("dadi")==true );
	//	assert(isPinyin("money")==false );
	//	assert(isPinyin("hanguogonsi")==false );
	//	assert(isPinyin("移行abc")==false );

 	if (wordLogFreq_.num_items() == 0) { 
		std::string phraseDict = path_ + "/chn_pinyin_dict.txt";
     	std::string phrase;
		std::string pinyin;	
		ifstream phraseInput(phraseDict.c_str() );
 		while (phraseInput.good())
		{
		    phraseInput >> pinyin;
		    phraseInput >> phrase;
			//cout<<pinyin<<" -> "<<phrase<<endl;
 			izenelib::util::UString ukey = izenelib::util::UString(phrase,
 					izenelib::util::UString::UTF_8);
 			updateWordLog_(ukey, CQC_WORD_FACTOR); 			
 		}
 		wordLogFreq_.commit();
 	}

    if (ngramFreq_.num_items() != 0)
		return;

	DLOG(INFO)<<"ChineseQueryCorrection, train on corpus with directory="
			<<corpusDir<<endl;
	if (bfs::is_directory(corpusDir) == false) {
		DLOG(ERROR)<<"Bad corpus directory!"<<endl;
		return;
	}
	ngramFreq_.commit();
	trainCorpus_(corpusDir);
}

void updateNgramFreq(izenelib::am::sdb_fixedhash<uint64_t, uint32_t>& sdb,
		const izenelib::util::UString& gram) {
	unsigned int freq = 0;
	uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(gram);
	sdb.get(hkey, freq);
	sdb.update(hkey, freq+1);
}

void ChineseQueryCorrection::trainCorpus_(const std::string& corpusDir) {

	static const bfs::directory_iterator kItrEnd;
	izenelib::am::sdb_fixedhash<uint64_t, uint32_t> unigram("_unigram_stat_");
	izenelib::am::sdb_fixedhash<uint64_t, uint32_t> bigram("_bigram_stat_");
	izenelib::am::sdb_fixedhash<uint64_t, uint32_t> trigram("_trigram_stat_");
	trigram.setDegree(17);
	trigram.setCacheSize(2000000);
	unigram.open();
	bigram.open();
	trigram.open();
	int cnt = 0;

	for (bfs::directory_iterator itr(corpusDir); itr != kItrEnd; ++itr) {
		string fileName = corpusDir+"/"+itr->path().filename();
		DLOG(INFO)<<"training ..."<<fileName<<endl;
		ifstream ifs(fileName.c_str() );
		string line;
		izenelib::util::UString uline;

		while (getline(ifs, line) ) {
			++cnt;
			if (cnt%100000 == 0) {
				DLOG(INFO)<<"process "<<cnt<<" lines"<<endl;
				unigram.commit();
				bigram.commit();
				trigram.commit();
			}
			uline = izenelib::util::UString(line, izenelib::util::UString::UTF_8);

			bool last1Chinese = false;
			bool last2Chinese = false;
			for (size_t i=0; i<uline.length(); i++) {
				bool isChinese = uline.isChineseChar(i);
				if (isChinese) {
					updateNgramFreq(unigram, uline.substr(i, 1) );
				}
				if (i>=1 && isChinese && last1Chinese) {
					updateNgramFreq(bigram, uline.substr(i-1, 2) );
				}
				if (i>=2 && isChinese && last1Chinese && last2Chinese) {
					updateNgramFreq(trigram, uline.substr(i-2, 3) );
				}
				last2Chinese = last1Chinese;
				last1Chinese = isChinese;
			}
		}//end while
	}

	izenelib::am::sdb_fixedhash<uint64_t, uint32_t>::SDBCursor locn;
	locn = unigram.get_first_locn();
	uint64_t k;
	uint32_t v;
	while (unigram.get(locn, k, v) ) {
		if (v>CQC_UNIGRAM_FACTOR+1)
			ngramFreq_.insert(k, v);
		unigram.seq(locn);
	}
	unigram.close();

	locn = bigram.get_first_locn();
	while (bigram.get(locn, k, v) ) {
		if (v>CQC_BIGRAM_FACTOR+1)
			ngramFreq_.insert(k, v);
		bigram.seq(locn);
	}
	bigram.close();

	locn = trigram.get_first_locn();
	while (trigram.get(locn, k, v) ) {
		if (v>CQC_TRIGRAM_FACTOR+1)
			ngramFreq_.insert(k, v);
		trigram.seq(locn);
	}
	trigram.close();
	ngramFreq_.flush();

	std::remove("_unigram_stat_");
	std::remove("_biigram_stat_");
	std::remove("_trigram_stat_");

	//	ifstream ifs(fileName.c_str() );
	//	string line;
	//	izenelib::util::UString uline;
	//
	//	while (getline(ifs, line) ) {
	//		uline = izenelib::util::UString(line, izenelib::util::UString::UTF_8);
	//
	//		bool last1Chinese = false;
	//		bool last2Chinese = false;
	//		for (size_t i=0; i<uline.length(); i++) {
	//			bool isChinese = uline.isChineseChar(i);
	//			if (isChinese)
	//				updateNgramFreq_(uline.substr(i, 1) );
	//			if (i>=1 && isChinese && last1Chinese) {
	//				//for bigram
	//				updateNgramFreq_(uline.substr(i-1, 2) );
	//			}
	//			if (i>=2 && isChinese && last1Chinese && last2Chinese) {
	//				//for trigram
	//				updateNgramFreq_(uline.substr(i-2, 3) );
	//			}
	//			last2Chinese = last1Chinese;
	//			last1Chinese = isChinese;
	//		}
	//	}//end while
	//	ngramFreq_.flush();
}

void ChineseQueryCorrection::updateNgramFreq_(const izenelib::util::UString& gram,
		int delta) {

	unsigned int freq = 0;
	uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(gram);
	ngramFreq_.get(hkey, freq);
	ngramFreq_.update(hkey, freq+delta);
}

void ChineseQueryCorrection::updateLogFreq_(const izenelib::util::UString& gram,
		int delta) {

	unsigned int freq = 0;
	uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(gram);
	wordLogFreq_.get(hkey, freq);
	wordLogFreq_.update(hkey, freq+delta);
}

unsigned int ChineseQueryCorrection::getNgramFreq_(const izenelib::util::UString& gram) {
	CREATE_SCOPED_PROFILER(ChineseQueryCorrection, "BA::QueryCorrection",
			"ChineseQueryCorrection::getFreq_");

	uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(gram);
	unsigned int freq = 0;
	ngramFreq_.get(hkey, freq);
	return freq;
}

unsigned int ChineseQueryCorrection::getLogFreq_(const izenelib::util::UString& gram) {
	CREATE_SCOPED_PROFILER(ChineseQueryCorrection, "BA::QueryCorrection",
			"ChineseQueryCorrection::getFreq_");

	uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(gram);
	unsigned int freq = 0;
	wordLogFreq_.get(hkey, freq);
	return freq;
}

void ChineseQueryCorrection::updateWordLog_(const izenelib::util::UString& word,
		int delta) {

	bool last2Chinese = false;
	bool last1Chinese = false;
	for (size_t i=0; i<word.length(); i++) {
		bool isChinese = word.isChineseChar(i);
		if (isChinese) {
			//          should we update Ngram freq at the same time? 
// 			updateNgramFreq_(word.substr(i, 1), delta*CQC_UNIGRAM_FACTOR);
			updateLogFreq_(word.substr(i, 1), delta);
		}
		if (i>=1 && isChinese && last1Chinese) {
			//for bigram		
// 			updateNgramFreq_(word.substr(i-1, 2), delta*CQC_BIGRAM_FACTOR);
			updateLogFreq_(word.substr(i-1, 2), delta);
		}
		if (i>=2 && isChinese && last1Chinese && last2Chinese) {
			//for trigram			
// 			updateNgramFreq_(word.substr(i-2, 3), delta*CQC_TRIGRAM_FACTOR);
			updateLogFreq_(word.substr(i-2, 3), delta);
		}
		last2Chinese = last1Chinese;
		last1Chinese = isChinese;
	}
}

//calcualte the trigram. eg:中国人 ＝中＋中国＋国人＋中国人
double ChineseQueryCorrection::getWordScore_(const izenelib::util::UString& word) {
	double score = 0.0;

	for (size_t i=0; i<word.length(); i++) {
		score += getScore_(word.substr(i, 1) );
		if (i>=1)
			score += getScore_(word.substr(i-1, 2) );
		if (i>=2)
			score += getScore_(word.substr(i-2, 3) );
	}
	return score;
}

double ChineseQueryCorrection::getGenScore_(const string& pinyin,
		const string& hanzi) {
	//	cout<<"!!! getGenScore_: "<<hanzi<<" - "<<pinyin<<endl;
	//	cout<<pinyinHanziStat_[make_pair(pinyin, hanzi)]<<" vs "<<pinyinStat_[pinyin]<<endl;
	return (log(pinyinHanziStat_[make_pair(pinyin, hanzi) ]+1) -log(pinyinStat_[pinyin]+1) )/3;
}

double ChineseQueryCorrection::getScore_(const izenelib::util::UString& gram) {
	//	avg: 5248.37
	//	2gram: 62627557  2103045
	//	avg: 29.7795
	//	3gram: 50390259  12724743
	//	avg: 3.96002


	unsigned int freq = ( 1+getNgramFreq_(gram) )*( 1+getLogFreq_(gram) );
	if (freq < 10 && gram.length() == 2)
		return -15.0;
	if (freq < 5 && gram.length() == 3)
		return -8.0;
	return log(freq)*gram.length();


}


//convert chinese query into pinyin list recursively. eg: 银行=>yinxing yinhang
void ChineseQueryCorrection::toPinyins(const string& query,
		std::vector<std::string>& pinyins) {
	pinyins.clear();
	if(query == "")
		return;
    UString ustr(query, UString::UTF_8);
	if (ustr.length() == 1) {
		if (ustr.isChineseChar(0) ) {
			string ch;
			ustr.substr(0,1).convertString(ch, izenelib::util::UString::UTF_8);
			for (size_t i=0; i<pinyinMap_[ch].size(); i++) {
				pinyins.push_back(pinyinMap_[ch][i]);
			}
		} else {
			if (query != " ")
				pinyins.push_back(query);
		}
		return;
	}

	int last = ustr.length()-1;
	assert(last> 0 );
	string str;
	ustr.substr(0, last).convertString(str, izenelib::util::UString::UTF_8);
	toPinyins(str, pinyins);
	string ch;
	ustr.substr(last,1).convertString(ch, izenelib::util::UString::UTF_8);
	if (ustr.isChineseChar(last) ) {
		std::vector<std::string> temp;
		for (size_t i=0; i<pinyinMap_[ch].size(); i++) {
			for (size_t j=0; j<pinyins.size(); j++) {
				temp.push_back(pinyins[j] + pinyinMap_[ch][i]);
			}
		}
		pinyins = temp;
	} else {
		for (size_t i=0; i<pinyins.size(); i++) {
			if (ch != " ")
				pinyins[i] = pinyins[i] + ch;
		}
	}



}
//convert chinese query into pinyin list recursively. eg: 银行=>yinxing yinhang
//along with fuzzy pinyin.eg: yinxing->yingxin, yinxing
void ChineseQueryCorrection::toPinyins_(const string& query,
		std::vector<std::string>& pinyins) {

	toPinyins(query, pinyins);
	//add fuzzy pinyin
	vector<string> temp = pinyins;
	int size = pinyins.size();
	for(int i=0;i<size;i++){
		vector<string> candidates;
		pn_.getEditCandidates(pinyins.at(i), candidates);
		temp.insert(temp.end(),candidates.begin(),candidates.end());
	}
    pinyins = temp;



}
bool ChineseQueryCorrection::isDictPinyin_(const string& pinyin) {
	if (pinyin == "n" || pinyin == "ng" || pinyin == "ang"|| pinyin == "m") {
		return false;
	}
	return chineseMap_.find(pinyin) != chineseMap_.end();
}

bool ChineseQueryCorrection::pinyinSegment(const string& str,
		std::vector<vector<string> >& result) {
	if (pinyinSegment_(str, result) )
		return true;

	vector<string> candidates;
	pn_.getEditCandidates(str, candidates);
	for (size_t i=0; i<candidates.size(); i++) {
		//cout<<"expand: "<<candidates[i]<<endl;
		if (pinyinSegment_(candidates[i], result) )
			return true;
	}
	return false;
}
//xiashui＝>xia shui | xia a shui |
bool ChineseQueryCorrection::pinyinSegment_(const string& str,
		std::vector<vector<string> >& result) {

	if (isDictPinyin_(str) ) {
		vector<string> hit;
		hit.push_back(str);
		result.push_back(hit);
		for (size_t pos=1; pos<str.length(); pos++) {
			string str1 = str.substr(0, pos);
			string str2 = str.substr(pos);
			//cout<<"pinyinsegment: "<<str1<<" "<<str2<<endl;
			if (isDictPinyin_(str1) && isDictPinyin_(str2) ) {
				hit.clear();
				hit.push_back(str1);
				hit.push_back(str2);
				result.push_back(hit);
			}
		}
		return true;
	} else {
		int pos=str.length()-1;
		if (pos <= 0)
			return false;
		result.clear();
		bool ret = false;
		for (int i=0; i<CQC_MAX_ONE_PINYIN_LENGTH; i++) {
			if (pos-i <= 0)
				break;
			string substr = str.substr(pos-i);
			if (isDictPinyin_(substr) ) {
				std::vector<vector<string> > partResult;
				string part = str.substr(0, pos-i);
				if (pinyinSegment_(part, partResult) ) {
					for (size_t j=0; j<partResult.size(); j++) {
						partResult[j].push_back(substr);
						result.push_back(partResult[j]);
					}
					ret =true;
				}
			}
		}
		return ret;
	}
}

void ChineseQueryCorrection::getBestPath_(const vector<string>& pinyins,
		const vector<vector<string> >& candidate,
		vector<boost::shared_ptr<CandidateScore> >& result) {
	//Viterbi
	PQTYPE pq;
	for (size_t i=0; i<candidate.size(); i++) {
		PQTYPE newpq;
		vector<CandidateScorePtr> part;
		while ( !pq.empty() ) {
			part.push_back(pq.top() );
			pq.pop();
		}
		assert( pq.empty() );
		for (size_t j=0; j<candidate[i].size(); j++) {
			izenelib::util::UString unigram = izenelib::util::UString(candidate[i][j],
					izenelib::util::UString::UTF_8);
			if (part.size() == 0) {

				boost::shared_ptr<CandidateScore> newitem(new CandidateScore);
				newitem->path = unigram;
				newitem->score = getScore_(unigram);
				newitem->score += getGenScore_(pinyins[i], candidate[i][j]);
				newpq.push(newitem);
			}
			for (size_t k=0; k<part.size(); k++) {
				boost::shared_ptr<CandidateScore> newitem(new CandidateScore);
				newitem->path = part[k]->path;
				newitem->score = part[k]->score;

				newitem->path += unigram;
				int pos = newitem->path.length()-2;
				assert(pos >= 0);
				izenelib::util::UString bigram = newitem->path.substr(pos, 2);
				assert( bigram.length() == 2 );

				//unsigned int bkey = izene_hashing(bigram);
				newitem->score += getScore_(unigram);
#ifdef DEBUGDEBUG

				if(getScore_(bigram)>0){
					cout<<"unigram "<<unigram<<" "<<getScore_(unigram)<<endl;
					cout<<"bigram "<<bigram<<" "<<getScore_(bigram)<<endl;
				}
#endif
				newitem->score += getGenScore_(pinyins[i], candidate[i][j]);
				int flag =1;
				if(getScore_(bigram)<0)
					flag =-1;
				newitem->score += getScore_(bigram);//*getScore_(bigram)*flag;
				if (pos >=1) {
					izenelib::util::UString trigram = newitem->path.substr(pos-1, 3);
					assert( trigram.length() == 3 );
					newitem->score += getScore_(trigram);//*getScore_(trigram)*getScore_(trigram);
				}
				newpq.push(newitem);
			}
		}
		int count = 0;
		while ( !newpq.empty() && count < TOPK) {
			izenelib::util::UString ustr = newpq.top()->path;
			pq.push(newpq.top() );
			newpq.pop();
			++count;
		}

	}

	int count = 1;
	while ( !pq.empty() && count < TOPK) {
		boost::shared_ptr<CandidateScore> item = pq.top();
		//divide length
		item->score = item->score/(item->path.length() );
		result.push_back(item);
//		item->display();
		pq.pop();
		++count;
	}
}

//given one pinyin, check if it exists in chn_pinyin_dict.txt.
//if it exists, find the result and return true. eg: yinxing =>隐形 银杏
bool ChineseQueryCorrection::findPinyinDict_(const std::string& query,
		std::vector<std::string>& result) {
	if (pinyin2phrase_.get(query, result) )
	{
#ifdef DEBUGDEBUG
		std::cout<<"first findPinyinDict_ query is "<<query<<" and result are: ";
		for(int i=0; i<result.size();i++){
				std::cout<<result.at(i)<<" ";
		}
		cout<<endl;
#endif
	}

	if(result.size()>0)
	{
		return true;
	}
	else
		return false;
	//return false;

}

bool ChineseQueryCorrection::getRefinedQuery(const std::string& query, 
		std::vector<string>& canadiates) {
	return getRefinedQuery("", query, canadiates);
}

bool ChineseQueryCorrection::getRefinedQuery(const std::string& collectionName, const UString& query, 
           UString& refinedQueryUString){
    vector<string> refinedStrings;
	string queryString;
	query.convertString(queryString, UString::UTF_8);
	if(getRefinedQuery(collectionName, queryString,
			refinedStrings) )
	 {
	     for(size_t i=0; i<refinedStrings.size();i++)
	     {	
		    refinedQueryUString += UString(refinedStrings[i], UString::UTF_8);
	     	refinedQueryUString +=  UString("|", UString::UTF_8);
	     }
		 return true;
	 }
	return false;
}
	


bool ChineseQueryCorrection::getRefinedQuery(const std::string& collectionName
, const std::string& query, std::vector<string>& canadiates)
{
    izenelib::util::ClockTimer timer;
    vector<string> pinyins;

    string lowerQuery = toLower(query);
    //convert query to pinyin list( with the help of hanzi-pinyin dict)
    //it has dealt with polyphemes eg: 银行->yinhang yiheng. but didn't deal with fuzzy pinyin
    toPinyins_(lowerQuery, pinyins);
#ifdef DEBUGDEBUG
    cout<<"toPinyins_ are: ";
    for(int i=0;i<pinyins.size(); i++){
    	cout<<pinyins.at(i)<<" ";
    }
    cout<<endl;
#endif
    vector<CandidateScorePtr> result;
    bool ret = false;

    //yinhang => yinhang  yinhan
    for (size_t i=0; i<pinyins.size(); i++) {

        string canadiate = pinyins[i];
        vector<string> dictPinyins;
        //for every pinyin candidate, find the chinese phrases. 比如 yinhang， 银行，引航
        bool isNotEmpty =findPinyinDict_(canadiate, dictPinyins);
        if (isNotEmpty) {
			
            /*if (dictPinyins.size() == 1) {
                canadiates.push_back(dictPinyins[0]);//呼吁 huyu
                return true; //this is not true for Polyphones
            }*/
        	//for every chinese phrase candidate found in dict, calculate its score.
            for (size_t i=0; i<dictPinyins.size(); i++) {
                CandidateScorePtr one(new CandidateScore);
                one->path = izenelib::util::UString(dictPinyins[i],
                        izenelib::util::UString::UTF_8);

                //how about long word
                if( collectionName== "" )
                {
                	one->score = getWordScore_(one->path)/one->path.length()*2;
                }
                else
                {
                	one->score = getWordScore_(collectionName, one->path)/one->path.length()*2;
                }
                result.push_back(one);
            }
            ret = true;
           // break;

        }
        //if we can't find candidate in chn_pinyin_dict.txt, we should segment the pinyin.
        else
        {
               continue;
               ///TODO TO Be optimized in future
               /// Commented By Yingfeng 2011-01-28
        	vector<vector<string> > pinyinSegments;
        	// rujia=> ru jia| ru ji a
			if (pinyinSegment(canadiate, pinyinSegments) )
				ret =true;
			else {
				continue;
			}


			for (size_t i=0; i<pinyinSegments.size(); i++) {
	#ifdef DEBUGDEBUG
				cout<<"pinyin segments are: ";
				for (size_t j=0; j<pinyinSegments[i].size(); j++)
					cout<<pinyinSegments[i][j]<<" -> ";//ru->jia  ru->ji->a
				cout<<endl;
	#endif
			}
			size_t minSize = 16;
			//vector<string> pinyinSeg;

			if (pinyinSegments.size()< minSize){
			   minSize = pinyinSegments.size();
			}


			for (size_t i=0; i<minSize; i++) {
				vector<vector<string> > candidate;
				vector<string> pinyinSeg = pinyinSegments.at(i);

#ifdef DEBUGDEBUG
				cout<<"pinyinSeg size is "<<pinyinSeg.size()<<endl;
#endif
				for (size_t j=0; j<pinyinSeg.size(); j++) {

				   vector<string> words = chineseMap_.find(pinyinSeg[j])->second;//ru-> 如 入 辱。。。
				   #ifdef DEBUGDEBUG

				   	   	   	   if(!words.empty())
				   				{
				   	   	   		   cout<<"pinyin segments words ";
				   	   	   		   for (size_t m=0; m<words.size(); m++)
				   	   	   			   cout<<words[m]<<" -> ";
									cout<<endl;
				   				}

				   	#endif
				 /*  vector<string> strResult;
				   //for xian, ??also added
				   if (pinyin2phrase_.get(pinyinSeg[j], strResult) )
					 for(size_t k=0; k<strResult.size(); k++){
						if(std::find(words.begin(),words.end(),strResult[k]) == words.end() )
						  words.push_back( strResult[k] );
					 }*/
				   candidate.push_back(words);
				}
				#ifdef DEBUGDEBUG
				   cout<<"candiates are ";
				   for (size_t m=0; m<candidate.size(); m++)
				   {
					   for(size_t n=0;n<candidate.at(m).size();n++){
						   cout<<candidate.at(m).at(n)<<" ";
					   }
					   cout<<endl;
				   }
				#endif
				getBestPath_(pinyinSeg, candidate, result);

			}

        }

        
    }

    //common hanzi boost
  //  cout<<"after getBestPath_ :";
    for (size_t i=0; i<result.size(); i++) {
#ifdef DEBUGDEBUG
        string stringPath;
        result[i]->path.convertString(stringPath, izenelib::util::UString::UTF_8);
        cout<<"idx"<<i<<" ";
        cout<<stringPath<<" ";
        cout<<result[i]->score<<endl;
        //cout<<"\n==============="<<endl;
#endif
        //hanziBoost_(query, result[i]);
    }

    std::sort(result.begin(), result.end(), compareCandidateScore);
    double lastScore = 0.0;
    string lastStringPath = "";
    //only top3
    for (size_t i=0; i<result.size() && i<CQC_NBEST_QUERY_CORRECTION; i++) {
        string stringPath;
        result[i]->path.convertString(stringPath, izenelib::util::UString::UTF_8);

#ifdef DEBUGDEBUG
        cout<<"idx"<<i<<" ";
        cout<<stringPath<<endl;
        cout<<result[i]->score<<endl;
#endif

        if (stringPath == query)
        {
        	//canadiates.push_back(stringPath);
        	return true; //delete or not????
        }

        if (result[i]->score> lastScore*0.9 && stringPath != lastStringPath) {
        //if (stringPath != lastStringPath) {
        	 canadiates.push_back(stringPath);
        } else
        {
        	//break;
        	continue;
        }
        lastScore = result[i]->score;
        lastStringPath = stringPath;
    }

    DLOG(INFO)<<"One Chinese Query correction through HMM model elapased "
            <<timer.elapsed() <<"secondes."<<endl;
    //REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.BAProcess" );
    return ret;
}

void ChineseQueryCorrection::updateNgram(
		const std::list<std::pair<izenelib::util::UString,uint32_t> >& queryList) {
	updateNgram("", queryList);
}
void ChineseQueryCorrection::updateNgram(const std::string& collectionName, const std::list<std::pair<izenelib::util::UString,uint32_t> >& queryList)
{
    DLOG(INFO)<<"ChineseQueryCorrection, updateNgram..."<<endl;
    std::list<std::pair<izenelib::util::UString,uint32_t> >::const_iterator lit =
            queryList.begin();
    for (; lit != queryList.end(); lit++) {
        if( collectionName == "" )
        {
            updateWordLog_(lit->first, (int)(lit->second));
        }
        else
        {
            updateWordLog_(collectionName, lit->first, (int)(lit->second));
        }
    }
}

void ChineseQueryCorrection::hanziBoost_(const string&queryString,
		CandidateScorePtr& ptr) {
	izenelib::util::UString uqueryString = izenelib::util::UString(queryString,
			izenelib::util::UString::UTF_8);
	int matchNum = 0;
	for (size_t i=0; i<ptr->path.length(); i++) {
		if (ptr->path.isChineseChar(i) ) {
			if (uqueryString.find(ptr->path.substr(i, 1) )
					!= izenelib::util::UString::NOT_FOUND)
				++matchNum;
		}
	}
	ptr->score += matchNum*CQC_COMMON_HANZI_FACTOR;
}

bool ChineseQueryCorrection::getPinyin(const std::string& hanzis,
		std::vector<string>& pinyin) {
	toPinyins_(hanzis, pinyin);
	if (pinyin.size() == 0)
		return false;
	return true;
}

bool ChineseQueryCorrection::getPinyin(const izenelib::util::UString& hanzis,
		std::vector<izenelib::util::UString>& pinyin)
{
	string str;
	std::vector<std::string> pinyinRes;
	hanzis.convertString(str, izenelib::util::UString::UTF_8);
	if(!getPinyin(str, pinyinRes))
	    return false;
	for(uint32_t i=0;i<pinyinRes.size();i++)
	{
	    izenelib::util::UString ustr(pinyinRes[i], izenelib::util::UString::UTF_8);
	    pinyin.push_back(ustr);
	}
	return true;
}

bool ChineseQueryCorrection::isPinyin(const string& str) {
	int pos = str.length()-CQC_MAX_ONE_PINYIN_LENGTH;
	size_t off = CQC_MAX_ONE_PINYIN_LENGTH;
	if (pos <= 0) {
		pos = 0;
		off = str.length();
	}
	while (pos >= 0) {
		while ( !isDictPinyin_(str.substr(pos, off) ) ) {
			++pos;
			--off;
			if (off <=0)
				return false;
		}
		if (pos == 0)
			break;
		if (CQC_MAX_ONE_PINYIN_LENGTH >= pos) {
			off = pos;
			pos = 0;
		} else {
			off = CQC_MAX_ONE_PINYIN_LENGTH;
			pos = pos - CQC_MAX_ONE_PINYIN_LENGTH;
		}
	}
	return true;
}


void ChineseQueryCorrection::updateNgramFreq_(const std::string& collectionName, const izenelib::util::UString& gram, int delta)
{
//     unsigned int freq = 0;
    izenelib::util::UString ustr( collectionName+"#", izenelib::util::UString::UTF_8);
    ustr.append( gram );
    uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(ustr);
//     ngramFreq_.get(hkey, freq);
    ngramFreq_.update(hkey, delta);
}
void ChineseQueryCorrection::updateLogFreq_(const std::string& collectionName, const izenelib::util::UString& word, int delta)
{
//     unsigned int freq = 0;
    izenelib::util::UString ustr( collectionName+"#", izenelib::util::UString::UTF_8);
    ustr.append( word );
    uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(ustr);
//     wordLogFreq_.get(hkey, freq);
    wordLogFreq_.update(hkey, delta);
}
void ChineseQueryCorrection::updateWordLog_(const std::string& collectionName, const izenelib::util::UString& word, int delta)
{
    bool last2Chinese = false;
    bool last1Chinese = false;
    for (size_t i=0; i<word.length(); i++) {
        bool isChinese = word.isChineseChar(i);
        if (isChinese) {
            //          should we update Ngram freq at the same time? 
//             updateNgramFreq_(collectionName, word.substr(i, 1), delta*CQC_UNIGRAM_FACTOR);
            updateLogFreq_(collectionName, word.substr(i, 1), delta);
        }
        if (i>=1 && isChinese && last1Chinese) {
            //for bigram        
//             updateNgramFreq_(collectionName, word.substr(i-1, 2), delta*CQC_BIGRAM_FACTOR);
            updateLogFreq_(collectionName, word.substr(i-1, 2), delta);
        }
        if (i>=2 && isChinese && last1Chinese && last2Chinese) {
            //for trigram           
//             updateNgramFreq_(collectionName, word.substr(i-2, 3), delta*CQC_TRIGRAM_FACTOR);
            updateLogFreq_(collectionName, word.substr(i-2, 3), delta);
        }
        last2Chinese = last1Chinese;
        last1Chinese = isChinese;
    }
}
unsigned int ChineseQueryCorrection::getNgramFreq_(const std::string& collectionName, const izenelib::util::UString& gram)
{
    CREATE_SCOPED_PROFILER(ChineseQueryCorrection, "BA::QueryCorrection",
            "ChineseQueryCorrection::getFreq_");

    izenelib::util::UString ustr( collectionName+"#", izenelib::util::UString::UTF_8);
    ustr.append( gram );
    uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(ustr);
    unsigned int freq = 0;
    ngramFreq_.get(hkey, freq);
    hkey = HashFunction<izenelib::util::UString>::generateHash64(gram);
    unsigned int common_freq = 0;
    ngramFreq_.get(hkey, common_freq);
    freq += common_freq;
    return freq;
}
unsigned int ChineseQueryCorrection::getLogFreq_(const std::string& collectionName, const izenelib::util::UString& word)
{
    CREATE_SCOPED_PROFILER(ChineseQueryCorrection, "BA::QueryCorrection",
            "ChineseQueryCorrection::getFreq_");

    izenelib::util::UString ustr( collectionName+"#", izenelib::util::UString::UTF_8);
    ustr.append( word );
    uint64_t hkey = HashFunction<izenelib::util::UString>::generateHash64(ustr);
    unsigned int freq = 0;
    wordLogFreq_.get(hkey, freq);
    hkey = HashFunction<izenelib::util::UString>::generateHash64(word);
    unsigned int common_freq = 0;
    wordLogFreq_.get(hkey, common_freq);
    freq += common_freq;
    return freq;
}
double ChineseQueryCorrection::getScore_(const std::string& collectionName, const izenelib::util::UString& gram)
{
    unsigned int freq = ( 1+getNgramFreq_(collectionName, gram) )*( 1+getLogFreq_(collectionName, gram) );
    if (freq < 10 && gram.length() == 2)
        return -15.0;
    if (freq < 5 && gram.length() == 3)
        return -8.0;
    return log(freq)*gram.length();
}
double ChineseQueryCorrection::getWordScore_(const std::string& collectionName, const izenelib::util::UString& word)
{
    double score = 0.0;
    for (size_t i=0; i<word.length(); i++) {
        score += getScore_(collectionName, word.substr(i, 1) );//sqrt() should we reduce the probability of single character's unigram????
        if (i>=1)
            score += getScore_(collectionName, word.substr(i-1, 2) );
        if (i>=2)
            score += getScore_(collectionName, word.substr(i-2, 3) );
    }
    return score;
}

void ChineseQueryCorrection::convertToPinyinList(const izenelib::util::UString& ustr, std::vector<std::string>& pinyins)
{
    pinyins.clear();
    if (ustr.length()==0)
        return;
    
    if (ustr.length() == 1) {
        if (ustr.isChineseChar(0) ) {
            string ch;
            ustr.convertString(ch, izenelib::util::UString::UTF_8);
            for (size_t i=0; i<pinyinMap_[ch].size(); i++) {
                pinyins.push_back(pinyinMap_[ch][i]);
            }
        } else {
            if (ustr != izenelib::util::UString(" ", izenelib::util::UString::UTF_8))
            {
                string ch;
                ustr.convertString(ch, izenelib::util::UString::UTF_8);
                pinyins.push_back(ch);
            }
        }
        return;
    }

    int last = ustr.length()-1;
    assert(last> 0 );
    izenelib::util::UString subustr = ustr.substr(0, last);
    convertToPinyinList(subustr, pinyins);
    string ch;
    ustr.substr(last,1).convertString(ch, izenelib::util::UString::UTF_8);
    if (ustr.isChineseChar(last) ) {
        std::vector<std::string> temp;
        for (size_t i=0; i<pinyinMap_[ch].size(); i++) {
            for (size_t j=0; j<pinyins.size(); j++) {
                temp.push_back(pinyins[j] + pinyinMap_[ch][i]);
            }
        }
        pinyins = temp;
    } else {
        for (size_t i=0; i<pinyins.size(); i++) {
            if (ch != " ")
                pinyins[i] = pinyins[i] + ch;
        }
    }
}

bool ChineseQueryCorrection::findPinyinDict(const std::string& pinyin, std::vector<izenelib::util::UString>& result)
{
    std::vector<std::string> strResult;
    if ( !pinyin2phrase_.get(pinyin, strResult) )
    {
        vector<string> candidates;
        pn_.getEditCandidates(pinyin, candidates);

        vector<string>::iterator it=candidates.begin();
        for (; it != candidates.end(); it++) {
            if (pinyin2phrase_.get(*it, strResult) )
            {
                break;
            }
        }
        
    }
    if( strResult.size() == 0 ) return false;
    result.resize( strResult.size() );
    for(uint32_t i=0; i<result.size();i++)
    {
        result[i] = izenelib::util::UString(strResult[i], izenelib::util::UString::UTF_8);
    }
    return true;

    
}

double ChineseQueryCorrection::getWordScore(const izenelib::util::UString& word) {
    double score = 0.0;

    for (size_t i=0; i<word.length(); i++) {
        score += getScore_(word.substr(i, 1) );
        if (i>=1)
            score += getScore_(word.substr(i-1, 2) );
        if (i>=2)
            score += getScore_(word.substr(i-2, 3) );
    }
    return score;
}

void ChineseQueryCorrection::getChineseMap(const std::string& pinyin, std::vector<izenelib::util::UString>& result)
{
    vector<string> words = chineseMap_[pinyin ];
    result.resize( words.size() );
    for( uint32_t i=0;i<words.size();i++)
    {
        result[i] = izenelib::util::UString(words[i], izenelib::util::UString::UTF_8);
    }
}

double ChineseQueryCorrection::getScore(const izenelib::util::UString& gram) {
    //  avg: 5248.37
    //  2gram: 62627557  2103045
    //  avg: 29.7795
    //  3gram: 50390259  12724743
    //  avg: 3.96002


    unsigned int freq = ( 1+getNgramFreq_(gram) )*( 1+getLogFreq_(gram) );
    if (freq < 10 && gram.length() == 2)
        return -15.0;
    if (freq < 5 && gram.length() == 3)
        return -8.0;
    return log(freq)*gram.length();

    //  unsigned int freq = getFreq_(gram);
    //  if (freq <2 && gram.length() == 1)
    //      freq = 5248;
    //      //return -30.0;
    //  if (freq < 30 && gram.length() == 2)
    //      freq = 30;
    //      //return -30.0;
    //  if (freq < 4 && gram.length() == 3)
    //      freq = 4;
    //return -20.0;
    //  return log(freq)*gram.length();

}

double ChineseQueryCorrection::getScore(const std::string& collectionName, const izenelib::util::UString& gram)
{
    unsigned int freq = ( 1+getNgramFreq_(collectionName, gram) )*( 1+getLogFreq_(collectionName, gram) );
    if (freq < 10 && gram.length() == 2)
        return -15.0;
    if (freq < 5 && gram.length() == 3)
        return -8.0;
    return log(freq)*gram.length();
}

double ChineseQueryCorrection::getWordScore(const std::string& collectionName, const izenelib::util::UString& word)
{
    double score = 0.0;
    for (size_t i=0; i<word.length(); i++) {
        score += getScore_(collectionName, word.substr(i, 1) );
        if (i>=1)
            score += getScore_(collectionName, word.substr(i-1, 2) );
        if (i>=2)
            score += getScore_(collectionName, word.substr(i-2, 3) );
    }
    return score;
}

double ChineseQueryCorrection::getGenScore(const string& pinyin,
        const izenelib::util::UString& uhanzi) {
    //  cout<<"!!! getGenScore_: "<<hanzi<<" - "<<pinyin<<endl;
    //  cout<<pinyinHanziStat_[make_pair(pinyin, hanzi)]<<" vs "<<pinyinStat_[pinyin]<<endl;
    std::string hanzi;
    uhanzi.convertString(hanzi, izenelib::util::UString::UTF_8);
    return (log(pinyinHanziStat_[make_pair(pinyin, hanzi) ]+1) -log(pinyinStat_[pinyin]+1) )/3;
}

}

