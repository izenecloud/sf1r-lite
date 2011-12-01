#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_LDA_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_LDA_H_

#include "ts_common.h"

class Corpus;

class TopicModelLDA{

public:

    TopicModelLDA()
        : nTopics(0)
        , nDocs(0)
        , nWords(0)
        , nTotal(0)
        , alpha(0)
        , beta(0)
        , m_pDocOffs(NULL)
        , m_pWords(NULL)
        , m_pZ(NULL)
        , m_pNz(NULL)
        , m_mNdz(NULL)
        , m_mNwz(NULL)
        , m_mPdz(NULL)
        , m_mPwz(NULL)
        , m_bFromFile(false)
    {}

    explicit TopicModelLDA(int topic, double alp = 0.01, double bet = 0.01)
        : nTopics(topic)
        , nDocs(0)
        , nWords(0)
        , nTotal(0)
        , alpha(alp)
        , beta(bet)
        , m_pDocOffs(NULL)
        , m_pWords(NULL)
        , m_pZ(NULL)
        , m_pNz(NULL)
        , m_mNdz(NULL)
        , m_mNwz(NULL)
        , m_mPdz(NULL)
        , m_mPwz(NULL)
        , m_bFromFile(false)
    {}

    ~TopicModelLDA()
    {
        if (m_bFromFile)
        {
            if (m_pDocOffs != NULL)
                free(m_pDocOffs);
            if (m_pWords != NULL)
                free(m_pWords);
        }

        if (m_pZ != NULL)
            free(m_pZ);

        if (m_pNz != NULL)
            free(m_pNz);

        if (m_mNdz != NULL && m_mNdz[0] != NULL)
        {
            delete[] m_mNdz[0];
            delete[] m_mNdz;
        }

        if (m_mNwz != NULL && m_mNwz[0] != NULL)
        {
            delete[] m_mNwz[0];
            delete[] m_mNwz;
        }

        if (m_mPdz != NULL && m_mPdz[0] != NULL)
        {
            delete[] m_mPdz[0];
            delete[] m_mPdz;
        }

        if (m_mPwz != NULL && m_mPwz[0] != NULL)
        {
            delete[] m_mPwz[0];
            delete[] m_mPwz;
        }
    }

    /**
     *
     * @brief Load corpus from a continous buffer
     */
    void        load_corpus(int* buff);

    /**
     * @brief Load corpus from a corpus sturcture
     */
    void        load_corpus(Corpus* pCorpus);

    void        initialize();

    void        gibbs_sampling(int iter);

    double      loglikelihood();

    void        estimate_probs();

    int**       get_matrix_ndz(){return m_mNdz;}

    int**       get_matrix_nwz(){return m_mNwz;}

    int*        get_nz(){return m_pNz;}

    double**    get_matrix_pdz(){return m_mPdz;}

    double**    get_matrix_pwz(){return m_mPwz;}

    int*        get_topic_assinments(){return m_pZ;}

    void        write_to_file(const char* file);
    void        read_from_file(const char* file);
    void        print_stat();
    void        checksum();

    int         get_ntopics(){return nTopics;}
    int         get_ndocs(){return nDocs;}
    int         get_nwords(){return nWords;}
    int         get_ntotal(){return nTotal;}

private:

    void        mem_alloc();
    int         sample(double* probs, double tt);


private:
    int         nTopics;
    int         nDocs;
    int         nWords;
    int         nTotal;
    double      alpha;
    double      beta;

    //corpus
    int*        m_pDocOffs; ///< m_pDocOffs[d]: start word offset of document d
    int*        m_pWords; /// m_pWords[i] i-th word id

    //assignments
    int*        m_pZ;  ///< m_mDW[i]  i-th topic id
    int*        m_pNz;

    //model counts
    int**       m_mNdz;
    int**       m_mNwz;

    //probs
    double**    m_mPdz;
    double**    m_mPwz;


private:
    bool        m_bFromFile;

};

#endif // LDA_H_INCLUDED
