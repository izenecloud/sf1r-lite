#ifndef VERTEX_H
#define VERTEX_H

#include "ts_types.h"

//using namespace std;

namespace sf1r
{
namespace text_summarization
{

class Vertex
{
public:
    Vertex();
    Vertex(long nID, const Sentence& strSentence);
    virtual ~Vertex();

    Vertex& operator = (const Vertex&  otherVertex);
    bool operator == (const Vertex&  otherVertex);

    void setId(long ID);
    long getId()const ;

    void setSentence(Sentence& sentence);
    Sentence getSentence() const ;

    void setPageRank(double pageRank);
    double getPageRank() const;

private:
    long nId_; //the identifier;
    Sentence  strSentence_; // The sentence represented by the vertex.
    double  pageRank_; // The page rank score.

};

}
}

#endif
