/**
 * @file CategoryClassifyTable.h
 * @brief a table which stores one classified category for each doc.
 */

#ifndef SF1R_CATEGORY_CLASSIFY_TABLE_H
#define SF1R_CATEGORY_CLASSIFY_TABLE_H

#include <common/PropSharedLock.h>
#include <common/inttypes.h>
#include <string>
#include <vector>

namespace sf1r
{

class CategoryClassifyTable : public PropSharedLock
{
public:
    typedef std::string category_t;

    CategoryClassifyTable(
        const std::string& dirPath,
        const std::string& propName);

    bool open();
    bool flush();

    const std::string& propName() const { return propName_; }
    std::size_t docIdNum() const { return categories_.size(); }

    void resize(std::size_t num);
    void setCategory(docid_t docId, const category_t& category);

    /**
     * @brief get classified category for @p docId (has lock version).
     *
     * before calling this function, the caller does not need to acquire
     * the read lock, as it would acquire the lock by itself.
     */
    const category_t& getCategoryHasLock(docid_t docId) const;

    /**
     * @brief get classified category for @p docId (no lock version).
     *
     * before calling this function, in order to ensure safe concurrent access,
     * the caller must acquire the read lock first, just like below:
     *
     * <code>
     * CategoryClassifyTable::ScopedReadLock lock(CategoryClassifyTable::getMutex());
     * </code>
     */
    const category_t& getCategoryNoLock(docid_t docId) const;

private:
    const std::string dirPath_;

    const std::string propName_;

    std::vector<category_t> categories_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_CLASSIFY_TABLE_H
