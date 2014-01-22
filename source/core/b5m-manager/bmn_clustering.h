
//automatic brand,model,ngram clustering
class BmnClustering
{
    typedef BmnContainer::Value Value;
    typedef BmnContainer::ValueArray ValueArray;
    typedef boost::function<void (Document&)> Func;

public:
    BmnClustering(const std::string& path);
    ~BmnClustering();
    void AddCategoryKeyword(const std::string& keyword);
    void AddCategoryRule(const std::string& category_regex, const std::string& group_name);
    bool IsValid(const Document& doc) const;
    bool Append(const Document& doc);
    bool Finish(const Func& func);

private:
    std::string path_;
    Func func_;
};

