#include "Vertex.h"

namespace sf1r
{
namespace text_summarization
{


Vertex::Vertex()
{
    pageRank_ = 1;
}

Vertex::Vertex(long nID, const Sentence& strSentence)
{
    nId_ = nID;
    strSentence_ = strSentence;
    pageRank_ = 1.0;
}

Vertex::~Vertex()
{

}

long Vertex::getId() const
{
    return nId_;
}

Sentence Vertex::getSentence() const
{
    return strSentence_;
}

double Vertex::getPageRank() const
{
    return pageRank_;

}

void Vertex::setId(long ID)
{
    nId_ = ID;
}

void Vertex::setSentence(Sentence& sentence)
{
    strSentence_ = sentence;
}

void Vertex::setPageRank(double pageRank)
{
    pageRank_ = pageRank;
}

Vertex& Vertex::operator = (const Vertex& otherVertex)
{
    pageRank_ = otherVertex.getPageRank();
    nId_ = otherVertex.getId();
    strSentence_ = otherVertex.getSentence();
    return *this;
}

bool Vertex::operator == (const Vertex& otherVertex)
{
    return (this->nId_ == otherVertex.nId_);
}

}
}
