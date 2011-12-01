#include "stopword.h"
#include "ts_common.h"

namespace sf1r
{

Stopword *Stopword::INSTANCE = 0;
Stopword::HashSet Stopword::m_oStopwordSet;

void Stopword::load(const std::string& file)
{
    std::ifstream ifs(file.data());
    std::string line;
    m_oStopwordSet.clear();
    if (!ifs) return;
    while (getline(ifs,line))
        m_oStopwordSet.insert(line);
}

bool Stopword::is_stop_word(const std::string& word)
{
    return m_oStopwordSet.find(word) != m_oStopwordSet.end();
}

}
