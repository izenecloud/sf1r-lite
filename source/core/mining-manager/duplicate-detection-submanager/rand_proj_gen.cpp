/**
   @file rand_proj_gen.cpp
   @author Kevin Hu
   @date 2009.11.24
 */
#include "rand_proj_gen.h"
#include "IntergerHashFunction.h"
#include <util/hashFunction.h>

namespace sf1r
{

/********************************************************************************
Description: RandomeProjectionEngine projects given a text/string or a token --
               token is a Rabin's 8byte fingerprint of a text/string -- into
               n dimensional vector. Each dimension is represented by a real number
               ranging from -1 to 1. The association from text/token to random
               projection vector is maintained in a hash table(EfficientLHT).
               Each dimension is represented by float instead of double
               to save space.
Comments   : For details, see "Finding Near-Duplicate Web Pages: A Large-Scale
               Evaluation of Algorithms" by Monika Henzinger.
             For further technical details, see "Similarity Estimation Techniques
               from Rounding Algorithms" by Moses S. Charikar.
History    : Kevin Hu                                       1/12/07
               Intial Revision
********************************************************************************/


// these are static to be shared by all RandomProjectEngine objects.
boost::minstd_rand RandProjGen::rangen(47u); // random number generator
boost::uniform_real<> RandProjGen::uni_real(-1., 1.); // uniform distribution in [-1,1]
boost::variate_generator<boost::minstd_rand&, boost::uniform_real<> > RandProjGen::U(rangen, uni_real);


RandProj* RandProjGen::map_find_(uint32_t token)
{
    uint32_t i= token%ENTRY_SIZE_;
    if (entry_.at(i)==NULL)
        return NULL;

    slot_t* slot = entry_.at(i);
    uint32_t t = slot->find(UNIT_STRUCT(token));
    if (t == slot_t::NOT_FOUND)
        return NULL;

    return (RandProj*)(slot->at(t).PROJ());
}

void RandProjGen::map_insert_(uint32_t token, RandProj* proj)
{
    uint32_t i= token%ENTRY_SIZE_;
    if (entry_.at(i)==NULL)
        entry_[i] = new slot_t();

    entry_[i]->push_back(UNIT_STRUCT(token, (uint64_t)proj));
}

void RandProjGen::map_save_()
{
    // FILE* f = fopen((nm_+".proj").c_str(), "w+");
//     IASSERT(f != NULL);
//     for (uint32_t i=0; i<ENTRY_SIZE_; ++i)
//     {
//       if (entry_.at(i)==NULL)
//         continue;

//       slot_t* slot = entry_.at(i);
//       for (uint32_t j=0 ;j<slot->length(); ++j)
//       {
//         uint64_t addr = ftell(f);
//         ((RandProj*)(slot->at(j).PROJ()))->save(f);
//         delete ((RandProj*)(slot->at(j).PROJ()));
//         (*slot)[j].PROJ_() = addr;
//       }
//     }
//     fclose(f);

//     f = fopen((nm_+".ent").c_str(), "w+");
//     IASSERT(f != NULL);
//     uint32_t i=0;
//     for (; i<ENTRY_SIZE_; ++i)
//     {
//       if (entry_.at(i)==NULL)
//         continue;

//       IASSERT(fwrite(&i, sizeof(uint32_t), 1, f) == 1);
//       entry_.at(i)->save(f);
//       delete entry_.at(i);
//       entry_[i] = NULL;
//     }
//     i = -1;
//     IASSERT(fwrite(&i, sizeof(uint32_t), 1, f) == 1);
//     fclose(f);

}

void RandProjGen::map_load_()
{
    // for (uint32_t i=0; entry_.length()<ENTRY_SIZE_; ++i)
//         entry_.push_back(NULL);

//     FILE* f = fopen((nm_+".ent").c_str(), "r");
//     if (f == NULL)
//     {
//       return;
//     }

//     uint32_t i=0;
//     IASSERT(fread(&i, sizeof(uint32_t), 1, f) == 1);
//     while(i != (uint32_t)-1)
//     {
//       slot_t* slot = new slot_t();
//       slot->load(f);
//       entry_[i] = slot;

//       IASSERT(fread(&i, sizeof(uint32_t), 1, f) == 1);
//     }

//     fclose(f);
//     f = fopen((nm_+".proj").c_str(), "r");
//     IASSERT(f != NULL);

//     for (uint32_t i=0; i<ENTRY_SIZE_; ++i)
//     {
//       if (entry_.at(i)==NULL)
//         continue;

//       slot_t* slot = entry_.at(i);
//       for (uint32_t j=0 ;j<slot->length(); ++j)
//       {
//         uint64_t addr = slot->at(j).PROJ();
//         RandProj* proj = new RandProj(nDimensions_) ;
//         fseek(f, addr, SEEK_SET);
//         proj->load(f);
//         (*slot)[j].PROJ_() = (uint64_t)proj;
//       }
//     }
//     fclose(f);
}

/********************************************************************************
@brief Given a token, get its random projection. First lookup the table
               if it was already built. If no cache found, it builds one and
               stores in the table and returns its reference.
  ********************************************************************************/
RandProj RandProjGen::get_random_projection(uint32_t token)
{
    RandProj proj(nDimensions_);
    uint32_t key = token;
    int n = nDimensions_ - 1;
    float fva[] = {-0.1f, 0.1f};
    while (n >= 0)
    {
        uint32_t tkey = key;
        for (int i = 0; i <32 && n >= 0; ++i)
        {
            proj[n] = fva[tkey & 0x01];
            tkey >>= 1;
            --n;
        }
        if (n >= 0)
        {
            //boost::mutex::scoped_lock lock(table_mutex_);
            key = int_hash::hash32shift(key);
            // if (key & 0x01)
//           for (int i = 0; i < 32 && n >= 0; ++i, --n)
//             proj[n] = 0;
        }
    }
    return proj;

    //const RandProj* rp = map_find_(token);//table.find(token);
//     if(rp) {
//       return *rp;
//     } else {
//       RandProj* proj =  new RandProj(nDimensions_);
//       uint32_t key = token;
//       int n = nDimensions_ - 1;
//       float fva[] = {-0.1f, 0.1f};
//       while(n >= 0) {
//         uint32_t tkey = key;
//         for (int i = 0; i < 32 && n >= 0; ++i) {
//           (*proj)[n] = fva[tkey & 0x01];
//           tkey >>= 1;
//           --n;
//         }
//         if(n >= 0) {
//           //boost::mutex::scoped_lock lock(table_mutex_);
//           key = int_hash::hash32shift(key);
//         }
//       }

    // map_insert_(token, proj);
//       const RandProj * r = map_find_(token);
//       IASSERT(r);
    //return *r;
    //    }
}


RandProj RandProjGen::get_random_projection(const std::string& token)
{
    uint32_t convkey= izenelib::util::HashFunction<std::string>::generateHash32(token);
    //TokenGen::mfgenerate_token(token.c_str(),token.size());
    return get_random_projection(convkey);
}

void RandProjGen::commit()
{
    map_save_();
    map_load_();
}

}


