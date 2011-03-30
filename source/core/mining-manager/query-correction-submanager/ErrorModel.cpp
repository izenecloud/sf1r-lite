/**
 * @brief  source file of class Error model, it is used to compute score of noisy chaneel error model.
 * @author Jinli Liu
 * @date Oct 15,2009
 * @version v2.0
 */
#include "ErrorModel.h"


typedef boost::tokenizer<boost::char_separator<char> >        tokenizerSeq;
namespace
{

int Max = 80000000;
float NORM = 1;
static  const float SMOOTH = 0.000001;
int BASE =97;


}

namespace sf1r
{

//read from plain matrix file and assign six matrix we used for error model
void ErrorModel::assignMatrix(const char* matrixFile)
{
    ifstream infile;
    infile.open(matrixFile, ifstream::in);
    boost::char_separator<char> sep(",");
    std::string str;
    int line = 0;
    int i = 0;
    if ( !infile.is_open() )
    {
        cout << "Error : File(" << matrixFile << ") is not opened." << endl;
        return;
    }
    while (infile.good())
    {
        getline(infile,str);
        tokenizerSeq tok(str, sep);
        if (line>=0 && line<26)
        {
            for (tokenizerSeq::iterator beg=tok.begin(); beg!=tok.end();++beg)
                sub[line][i++]=atoi((*beg).c_str());
            line++;
            continue;
        }
        if (line>=26 && line<52)
        {
            int newline = line-26;
            i = 0;
            for (tokenizerSeq::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
                rev[newline][i++] = atoi((*beg).c_str());
            line++;
            continue;
        }
        if (line>=52 && line<79)
        {
            int newline = line-52;
            i = 0;
            for (tokenizerSeq::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
                del[newline][i++] = atoi((*beg).c_str());
            line++;
            continue;
        }
        if (line>=79 && line<106)
        {
            int newline = line-79;
            i = 0;
            for (tokenizerSeq::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
                add[newline][i++] = atoi((*beg).c_str());
            line++;
            continue;
        }
        if (line == 106)
        {
            i = 0;
            for (tokenizerSeq::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
                charX[i++] = atoi((*beg).c_str());
            line++;
            continue;

        }
        if (line>=107 && line<133)
        {
            int newline = line-107;
            int i = 0;
            for (tokenizerSeq::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
                charXY[newline][i++] = atoi((*beg).c_str());
            line++;


        }

    }
    infile.close();

}

//compute which operations occured from T to C
void getOperations(const izenelib::util::UString& T, const izenelib::util::UString& C, std::vector<element> & operations)
{
    const unsigned int HEIGHT = C.length() + 1;
    const unsigned int WIDTH = T.length() + 1;
    unsigned int eArray[HEIGHT][WIDTH];//edit distance matrix
    unsigned int i; // index var. for the rows
    unsigned int j; // index var. for the columns
    eArray[0][0] = 0;// Initialization distance matrix
    for (i = 1; i < HEIGHT; i++)
        eArray[i][0] = i;

    for (j = 1; j < WIDTH; j++)
        eArray[0][j] = j;

    for (i = 1; i < HEIGHT; i++)
    {
        for (j = 1; j < WIDTH; j++)
        {
            eArray[i][j] = min(
                               eArray[i - 1][j - 1] +
                               (T[j-1] == C[i-1] ? 0 : 1),
                               min(eArray[i - 1][j] + 1, eArray[i][j - 1] + 1));
        }
    }

    i = HEIGHT-1;
    j = WIDTH-1;
    while (i!=0 || j!=0)
    {
        if (i!=0 && eArray[i][j]==eArray[i-1][j]+1)//deletion
        {
            element ele(i--,j,1);
            operations.push_back(ele);
            continue;
        }
        if (j!=0 && eArray[i][j]==eArray[i][j-1]+1)//insertion
        {
            element ele(i,j--,2);
            operations.push_back(ele);
            continue;
        }
        if (eArray[i][j] == eArray[i-1][j-1]+1)//substituation
        {
            element ele(i--,j--,3);
            operations.push_back(ele);
            continue;
        }
        if (eArray[i][j] == eArray[i-1][j-1])//no operations
        {
            i--;
            j--;

        }

    }

}
//the error score from T to C based on confusion error model
float ErrorModel::getErrorModel(const izenelib::util::UString& T, const izenelib::util::UString& C)
{
    vector<element> operations;

    getOperations(T,C,operations);

    float score = 1.0;
    int size = operations.size();
    for (int i=0; i<size; i++)
    {
        int pc = --operations[i].x;
        int pt = --operations[i].y;
        if (pc > ((int)C.length() - 1)||pc<0 || pt > ((int)T.length() -1)||pt<0)
            return 0;
        int pm = C[pc]-BASE;

        int ntp = T[pt]-BASE;
        if (    i != size-1               &&
                operations.size() > 1     &&
                operations[i].flag == 1   &&
                operations[i+1].flag == 2 &&
                C[pc] == T[operations[i+1].y-1])//reversion
        {
            if ( pc<(int)C.length()-1)
            {
                int pos = C[pc+1]-BASE;
                score *= (rev[pm][pos]+1) /
                         (charXY[pm][pos]+1);
            }
            break;
        }

        if (operations[i].flag == 1)
        {
            if (pc == 0)//deletion at 0
                score *= (NORM+ del[26][pm]) /
                         (charX[pm]);
            else  //deletion at other position
            {
                if (pc>0)
                {
                    int pos = C[pc-1]-BASE;
                    score *= (del[pos][pm]+NORM) /
                             ((charXY[pos][pm])+NORM);
                }
            }
        }

        if (operations[i].flag == 2)
        {
            if (pc == -1)//insertion at 0
                score *= NORM*add[26][ntp]/Max;
            else//insertion at other position
                score *= (NORM+add[pm][ntp]) /
                         (charX[pm]+NORM);

        }

        if (operations[i].flag == 3)//substituation
        {
            score *= (NORM+sub[ntp][pm]) /
                     (charX[pm]+NORM);
        }
    }

    return score;
}
}
