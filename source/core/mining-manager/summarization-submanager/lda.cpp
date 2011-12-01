/*
 * lda.cpp
 *
 *  Created on: 2010-10-16
 *      Author: jay
 */
#include "lda.h"
#include "corpus.h"

#include <cstring>

using namespace std;

const int IT = 10;

const double RAND_MAX_D = 1.0/(double)RAND_MAX;
/**
 * @brief parse the input buffer with the following structure:
 *     ----------------Head------------
 *       <number of documents = D>
 *       <number of words = W>
 *       <total word length = T>
 *     --------------------------------
 *       <start word offset of doc_1>     <----------------m_pDocOffsets
 *       <start word offset of doc_2>
 *               ......
 *       <start word offset of doc_D>
 *       <total word length (offset end)>
 *     ------------ (doc_1)-------------
 *       |          <word id>        |    <----------------m_pWords + m_pDocOffsets[0]
 *       |          ......           |
 *       |          ......           |
 *       |          <word id>        |
 *     ---------------------------------
 *                  .........
 *                  .........
 *                  .........
 *     ------------ (doc_D)-------------
 *       |          word id>        |   <-----------------m_pWords + m_pDocOffsets[D-1]
 *       |          ......          |
 *       |          ......          |
 *       |          <word id>       |
 *     ---------------------------------
 * This function parse the above stream and store the pointers
 * for quickly random access.
 */
void TopicModelLDA::load_corpus(int* buff)
{
    nDocs       = *buff ++;
    nWords      = *buff ++;
    nTotal      = *buff ++;
    m_pDocOffs  = buff;
    m_pWords    = buff + nDocs + 1;
}

void TopicModelLDA::load_corpus(Corpus* pCorpus)
{
    nDocs = pCorpus->ndocs();
    nWords = pCorpus->nwords();
    nTotal = pCorpus->ntotal();
    m_pDocOffs = pCorpus->get_doc_offs();
    m_pWords = pCorpus->get_word_seq();
}

void TopicModelLDA::mem_alloc()
{

    if (m_bFromFile)
    {
        m_pDocOffs = (int*) malloc((nDocs + 1) * sizeof(int));
        m_pWords = (int*) malloc(nTotal * sizeof(int));
    }

    m_pZ = (int*) malloc(nTotal * sizeof(int));
    m_pNz = (int*) malloc(nTopics * sizeof(int));

    int* intBuff;
    double* doubleBuff;

    intBuff = new int[nDocs * nTopics];
    doubleBuff = new double[nDocs * nTopics];
    m_mNdz = new int*[nDocs];
    m_mPdz = new double*[nDocs];
    for (int d = 0 ; d < nDocs; ++d, intBuff += nTopics, doubleBuff += nTopics)
    {
        m_mNdz[d] = intBuff;
        m_mPdz[d] = doubleBuff;
    }

    intBuff = new int[nWords * nTopics];
    doubleBuff = new double[nWords * nTopics];
    m_mNwz = new int*[nWords];
    m_mPwz = new double*[nWords];
    for (int w = 0 ; w < nWords; ++w, intBuff += nTopics, doubleBuff += nTopics)
    {
        m_mNwz[w] = intBuff;
        m_mPwz[w] = doubleBuff;
    }
}

void TopicModelLDA::initialize()
{

    mem_alloc();

    memset(*m_mNdz, 0, nDocs * nTopics * sizeof(int));
    memset(*m_mNwz, 0, nWords * nTopics * sizeof(int));
    memset(m_pNz, 0, nTopics * sizeof(int));

    int i = 0;
    for (int d = 0 ; d < nDocs ; ++d)
    {
        for (i = m_pDocOffs[d]; i < m_pDocOffs[d + 1]; ++i)
        {
            int w = m_pWords[i] ;//sample for <d,w>
            int z = rand() % nTopics;
            ++m_mNdz[d][z];
            ++m_mNwz[w][z];
            ++m_pNz[z];
            m_pZ[i] = z;
        }
    }
}

int TopicModelLDA::sample(double* probs, double tt)
{
    double r = rand() * RAND_MAX_D * tt;
    int i = 0;
    double currprob = probs[0];
    while (r > currprob)
    {
        ++i;
        currprob += probs[i];
    }
    return i;
}

void TopicModelLDA::gibbs_sampling(int iter)
{

//  srand((unsigned)time(NULL));

    double WB = nWords * beta;
    double *probs = new double[nTopics];
    for (int it =0 ; it < iter ; ++it)
    {
        int i = 0;
        for (int d = 0 ; d < nDocs ; ++d)
        {
            for (i = m_pDocOffs[d]; i < m_pDocOffs[d + 1]; ++i)
            {
                int w = m_pWords[i];
                int z_old = m_pZ[i];
                -- m_mNdz[d][z_old];
                -- m_mNwz[w][z_old];
                -- m_pNz[z_old];

                double total_len = 0;
                for (int zz = 0 ; zz < nTopics ; ++zz)
                {
                    double prob =       ((double)m_mNdz[d][zz] + alpha)
                            * ((double)m_mNwz[w][zz] + beta)
                            / ((double)m_pNz[zz] + WB );
                    total_len += prob;
                    probs[zz] = prob;
                }
                int z_new = sample(probs, total_len);

                ++m_mNdz[d][z_new];
                ++m_mNwz[w][z_new];
                ++m_pNz[z_new];

                if (z_new != z_old)
                    m_pZ[i] = z_new;
            }
        }
        if (it % IT == 0)
        {
            cout << "it = " << it << ", ";
            estimate_probs();
            loglikelihood();
        }
    }

    delete probs;
}

void TopicModelLDA::estimate_probs()
{

    //m_mPwz
    double WB = nWords * beta;
    for (int w = 0; w < nWords; ++w)
        for (int z = 0; z < nTopics; ++z)
            m_mPwz[w][z] = (m_mNwz[w][z] + beta) / ((double)m_pNz[z] + WB );

    //m_mPdz
    for (int d = 0; d < nDocs; ++d)
    {
        double sum = 0;
        for (int z = 0; z < nTopics; ++z)
        {
            m_mPdz[d][z] = (m_mNdz[d][z] + alpha);
            sum += m_mPdz[d][z];
        }
        sum = 1.0 / sum;
        for (int z = 0; z < nTopics; ++z)
            m_mPdz[d][z] *= sum;
    }

}

double TopicModelLDA::loglikelihood()
{
    double llk_w = 0;
    int i = 0 ;
    for (int d = 0 ; d < nDocs ; ++d)
    {
//      cout << "d = " << d << endl;

        for (i = m_pDocOffs[d]; i < m_pDocOffs[d + 1]; ++i)
        {
//          cout << "i= " << i << endl;
//          cout << "m_pDocOffs[d] = " << m_pDocOffs[d] << endl;

            int w = m_pWords[i];

//          cout << "m_pWords[i] = " << m_pWords[i] << endl;

            double Pr_w = 0;
            for (int z = 0 ; z < nTopics;  ++z)
            {
//              if (i == 237)
//              {
//                  cout << "m_mPdz[d][z]:";
//                  cout << m_mPdz[d][z] << endl;
//                  cout << "m_mPwz[w][z]:";
//                  cout << m_mPwz[w][z] << endl;
//              }

                Pr_w += m_mPdz[d][z] * m_mPwz[w][z];
            }
            llk_w += log(Pr_w);
        }
    }
    double frac_ntt = -1.0 / (double)nTotal;
    cout << " pplx_w = " << exp(llk_w * frac_ntt) << ", llk_w = " << llk_w  << endl;
    return llk_w;
}

void TopicModelLDA::write_to_file(const char* file)
{
    ofstream ofs(file);

    ofs.write((const char*)&nTopics, sizeof(int));
    ofs.write((const char*)&nDocs, sizeof(int));
    ofs.write( (const char*)&nWords, sizeof(int));
    ofs.write( (const char*)&nTotal, sizeof(int));
    ofs.write( (const char*)&alpha, sizeof(double));
    ofs.write( (const char*)&beta, sizeof(double));


    ofs.write( (const char*) m_pDocOffs, sizeof(int) * (nDocs + 1));
    ofs.write( (const char*) m_pWords, sizeof(int) * nTotal);
    ofs.write( (const char*) m_pZ, sizeof(int)* nTotal);
    ofs.write( (const char*) m_mNdz[0], sizeof(int) * nTopics * nDocs);
    ofs.write( (const char*) m_mNwz[0], sizeof(int) * nTopics * nWords);
    ofs.write( (const char*) m_pNz, sizeof(int) * nTopics);
    ofs.write( (const char*) m_mPdz[0], sizeof(double) * nTopics * nDocs);
    ofs.write( (const char*) m_mPwz[0], sizeof(double) * nTopics * nWords);

    ofs.close();

}

void TopicModelLDA::read_from_file(const char* file)
{

    m_bFromFile = true;

    ifstream ifs(file);

    ifs.read( (char*)  &nTopics, sizeof(int));
    ifs.read( (char*)  &nDocs, sizeof(int));
    ifs.read( (char*)  &nWords, sizeof(int));
    ifs.read( (char*)  &nTotal, sizeof(int));
    ifs.read( (char*)  &alpha, sizeof(double));
    ifs.read( (char*)  &beta, sizeof(double));

    mem_alloc();

    ifs.read( (char*)  m_pDocOffs, (nDocs + 1) * sizeof(int));
    ifs.read( (char*)  m_pWords, nTotal * sizeof(int));
    ifs.read( (char*)  m_pZ, nTotal * sizeof(int));
    ifs.read( (char*)  m_mNdz[0], nTopics * nDocs * sizeof(int));
    ifs.read( (char*)  m_mNwz[0], nTopics * nWords * sizeof(int));
    ifs.read( (char*)  m_pNz, nTopics * sizeof(int));
    ifs.read( (char*)  m_mPdz[0], nTopics * nDocs * sizeof(double));
    ifs.read( (char*)  m_mPwz[0], nTopics * nWords * sizeof(double));

    ifs.close();
}

void TopicModelLDA::print_stat()
{
    cout << "nTopics = " << nTopics << endl;
    cout << "nDocs = " << nDocs << endl;
    cout << "nWords = " << nWords << endl;
    cout << "nTotal = " << nTotal << endl;
    cout << "alpha = " << alpha << endl;
    cout << "beta = " << beta << endl;
}

void TopicModelLDA::checksum()
{
    double sum = 0;
    for (int w = 0 ; w < nWords; ++w)
    {

        for (int z = 0 ; z < nTopics; ++z)
            sum += m_mPwz[w][z];

    }

    cout << "sum = " << sum << endl;

}
