/**
 @file Chinese Query Correction
 @author Peisheng Wang
 
 @date 2010.02.25
 */

#ifndef CHINESE_QUERY_CORRECTION_H
#define CHINESE_QUERY_CORRECTION_H

#include <string>
#include <vector>
#include <util/ustring/UString.h>
#include <boost/unordered_map.hpp>
#include <boost/noncopyable.hpp>
#include <sdb/SequentialDB.h>

namespace sf1r {

struct CandidateScore {
	izenelib::util::UString path;
	double score;

	CandidateScore() :
		path(izenelib::util::UString("", izenelib::util::UString::UTF_8) ), score(0.0) {
	}

	friend bool operator <(const CandidateScore& e1, const CandidateScore& e2) {
		return (e1.score < e2.score);
	}

	void display() const {
		string strPath;
		path.convertString(strPath, izenelib::util::UString::UTF_8);
		cout<<"path: "<<strPath<<endl;
		cout<<"score: "<<score<<endl;
	}
};

typedef boost::shared_ptr<CandidateScore> CandidateScorePtr;

struct CandidateScoreFunctor {
	bool operator()(const CandidateScorePtr &lhs, const CandidateScorePtr &rhs) const {
		return (lhs->score < rhs->score);
	}
};

typedef std::priority_queue<CandidateScorePtr, std::vector<CandidateScorePtr>, CandidateScoreFunctor>
		PQTYPE;

bool compareCandidateScore(const CandidateScorePtr&, const CandidateScorePtr&);


class PinyinNeighbor {
	boost::unordered_map<string, string> pairs_;	
public:
	PinyinNeighbor()
	 {
		pairs_["l"] = "r";
		pairs_["c"] = "ch";
		pairs_["s"] = "sh";
		pairs_["z"] = "zh";
		pairs_["an"] = "ang";
		pairs_["en"] = "eng";
		pairs_["in"] = "ing";
		pairs_["on"] = "ong";

		pairs_["r"] = "l";
		/*pairs_["ch"] = "c";
		pairs_["sh"] = "s";
		pairs_["zh"] = "z";
		pairs_["ang"] = "an";
		pairs_["eng"] = "en";
		pairs_["ing"] = "in";
		pairs_["ong"] = "on";*/

	}

	/*bool getEditCandidates(const string& pinyin,  vector<string>& candidates) {
		boost::unordered_map<string, string>::iterator it=pairs_.begin();
		for (; it != pairs_.end(); it++) {
			size_t pos=0;
			while ( true) {
				pos=pinyin.find(it->first, pos);

				if (pos != string::npos) {
					string candidate = pinyin;
					candidate.replace(pos, it->first.length(), it->second);
					candidates.push_back(candidate);
					pos = pos + it->second.length();
				} else
					break;
			}
		}
		return true;
	}*/
	bool getEditCandidates(const string& pinyin,  vector<string>& candidates) {
		boost::unordered_map<string, string>::iterator it=pairs_.begin();
		//cout<<"entering getEditCandidates."<<endl;
		for (; it != pairs_.end(); it++) {
			size_t pos=0;
			size_t pos2 = 0;
			while ( true) {
				pos=pinyin.find(it->first, pos);//ang
				pos2 = pinyin.find(it->second,pos2);//an
				if (pos != string::npos) {//pos=1
					if(pos2 == pos){//replace second with first
						//how about zhonguo
						if(it->first !="on" )
						{
							string candidate = pinyin;//zangkuan
							candidate.replace(pos2, it->second.length(), it->first);//ang->an  zankuan
							candidates.push_back(candidate);
							pos = pos + it->second.length();
						}
						else{
							string candidate = pinyin;//zhonguo
							candidate.replace(pos2, it->first.length(), it->second);//zhongguo
							if(candidate.find("ggg")==string::npos)
								candidates.push_back(candidate);
							pos = pos + it->first.length();
						}

						pos2 = pos;
					}
					else{
						string candidate = pinyin;//zangkuan  只找到了an，没有找到ang
						candidate.replace(pos, it->first.length(), it->second);
						candidates.push_back(candidate);
						pos = pos + it->first.length();
						pos2 = pos;
					}

				}
				else
				{
						break;
				}
			}
		}
		//cout<<"leaving getEditCandidates."<<endl;
		return true;
	}


};
/**
 @class ChineseQueryCorrection
 */
class ChineseQueryCorrection : public boost::noncopyable {
public:
	enum {TOPK = 30};
	typedef izenelib::am::sdb_btree<std::string, std::vector<std::string> >
			SDBType;
	typedef SDBType::SDBCursor SDBCursor;
	typedef boost::unordered_map<std::string, std::vector<std::string> >
			DictType;
	typedef DictType::iterator DIT;

public:
	ChineseQueryCorrection(const string& path, const std::string& workingPath);
    
    ~ChineseQueryCorrection();
    
	bool getRefinedQuery(const std::string& query, 
			std::vector<std::string>& canadiates);
            
    bool getRefinedQuery(const std::string& collectionName, const std::string& query, 
            std::vector<std::string>& canadiates);

	bool getRefinedQuery(const std::string& collectionName, const UString& query, 
           UString& refinedUstring);
	
	void initialize();
	void train(const std::string& corpusDir);
	inline bool inChineseDict(const string& query) {
		bool b = false;
		return phrase2pinyin_.get(query, b);
	}
	inline bool inPinyinDict(const string& query) {
		std::vector<std::string> r;
		return pinyin2phrase_.get(query, r);
	}
	void updateNgram(const std::list<std::pair<izenelib::util::UString,uint32_t> >& queryList);
    void updateNgram(const std::string& collectionName, const std::list<std::pair<izenelib::util::UString,uint32_t> >& queryList);
	bool getPinyin(const std::string& hanzis,
			std::vector<string>& pinyin);
	bool getPinyin(const izenelib::util::UString& hanzis,
			std::vector<izenelib::util::UString>& pinyin);
	bool pinyinSegment(const string& str, std::vector<vector<string> >& result);
	bool isPinyin(const string& str);
	void warmUp();
    void convertToPinyinList(const izenelib::util::UString& ustr, std::vector<std::string>& pinyins);
    bool findPinyinDict(const std::string& pinyin, std::vector<izenelib::util::UString>& result);
    void getChineseMap(const std::string& pinyin, std::vector<izenelib::util::UString>& result);
    double getScore(const izenelib::util::UString& gram);
    double getScore(const std::string& collectionName, const izenelib::util::UString& gram);
    double getWordScore(const izenelib::util::UString& word);
    double getWordScore(const std::string& collectionName, const izenelib::util::UString& word);
    double getGenScore(const std::string& pinyin,
            const izenelib::util::UString& hanzi);
private:
	void toPinyins_(const string& str, std::vector<std::string>& pinyins);
	void toPinyins(const string& str, std::vector<std::string>& pinyins);
	bool pinyinSegment_(const string& str, std::vector<vector<string> >& result);
	void trainCorpus_(const std::string& fileName);
	void getBestPath_(const vector<string>& pinyins,
			const vector<vector<string> >& candidate,
			std::vector<boost::shared_ptr<CandidateScore> >& result);

	bool findPinyinDict_(const std::string& query,
			std::vector<std::string>& result);
            
    
private:
	inline void updateNgramFreq_(const izenelib::util::UString& word, int delta=1);
	inline void updateLogFreq_(const izenelib::util::UString& word, int delta=1);
	inline void updateWordLog_(const izenelib::util::UString& word, int delta=1);
	inline unsigned int getNgramFreq_(const izenelib::util::UString& word);
	inline unsigned int getLogFreq_(const izenelib::util::UString& word);
	inline bool isDictPinyin_(const string& pinyin);
	inline double getScore_(const izenelib::util::UString& gram);
	inline double getWordScore_(const izenelib::util::UString& word);
	inline double getGenScore_(const std::string& pinyin,
			const std::string& hanzi);

            
    //collection specific functions
    inline void updateNgramFreq_(const std::string& collectionName, const izenelib::util::UString& word, int delta=1);
    inline void updateLogFreq_(const std::string& collectionName, const izenelib::util::UString& word, int delta=1);
    inline void updateWordLog_(const std::string& collectionName, const izenelib::util::UString& word, int delta=1);
    inline unsigned int getNgramFreq_(const std::string& collectionName, const izenelib::util::UString& word);
    inline unsigned int getLogFreq_(const std::string& collectionName, const izenelib::util::UString& word);
    inline double getScore_(const std::string& collectionName, const izenelib::util::UString& gram);
    inline double getWordScore_(const std::string& collectionName, const izenelib::util::UString& word);
    
    
	//There may be hanzi in queryString. The correction canadidates with
	//the hanzi in queryString will have better ranking.
	inline void hanziBoost_(const string&queryString, CandidateScorePtr&);
	void updatePinyinStat_(const string& pinyin, const izenelib::util::UString &ustr);
private:
	std::string path_;
    std::string workingPath_;
    bool isOpen_;
	DictType pinyinMap_;//eg: <行，<xing,hang>>
	DictType chineseMap_;//eg: <xing，<行,星，形，幸.....>>
	boost::unordered_map<std::string, unsigned int> pinyinStat_;
	boost::unordered_map<std::pair<std::string,std::string>, unsigned int>
	pinyinHanziStat_;

// 	SDBType pinyin2phrase_;
// 	izenelib::am::sdb_btree<std::string> phrase2pinyin_;
    izenelib::am::rde_hash<std::string, std::vector<std::string> > pinyin2phrase_;
    izenelib::am::rde_hash<std::string, bool> phrase2pinyin_;
	izenelib::am::sdb_fixedhash<uint64_t, uint32_t, izenelib::util::ReadWriteLock> ngramFreq_;
	izenelib::am::sdb_fixedhash<uint64_t, uint32_t, izenelib::util::ReadWriteLock> wordLogFreq_;
	PinyinNeighbor pn_;
};

}//namespace


#endif
