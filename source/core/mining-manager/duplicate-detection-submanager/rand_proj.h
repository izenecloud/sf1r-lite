/**
   @file rand_proj.h
   @author Kevin Hu
   @date   2009.11.24
 */
#ifndef RAND_PROJ_H
#define RAND_PROJ_H


#include <am/graph_index/dyn_array.hpp>
#include <util/CBitArray.h>
#include <ostream>


namespace sf1r
{
/********************************************************************************
@class RandProj
@detail RandomProjection is a mapping from a token to random n dimension
                 vector whose elements ranges in [-1,1].
               For more info, see RandomProjectionEngine.cc
********************************************************************************/
class RandProj
{
private:
    izenelib::am::DynArray<float> proj_;//!< random projection is composed of -1. and 1.

public:

    /**
       @brief a constructor
       @param nDimension dimension of projection
     */
    explicit inline RandProj(int nDimensions)
            : proj_(nDimensions)
    {
        for (uint32_t i=0; i<(uint32_t)nDimensions;++i)
            proj_.push_back(0.);
    }

    /**
       @brief copy constructor
     */
    RandProj(const RandProj& other)
    {
        proj_ = other.proj_;
    }

    /**
       @brief copy function
     */
    RandProj& operator = (const RandProj& other)
    {
        proj_ = other.proj_;
        return *this;
    }


    /**
       @brief a destruction
     */
    inline ~RandProj() {}
public:
    /**
     * @brief read/write access to projection in d
     *
     * @param d subindex
     *
     * @return reference at d in projection
     */
    inline float& operator[](uint32_t d)
    {
        return proj_[d];
    }
    inline const float at(uint32_t d) const
    {
        return proj_.at(d);
    }
    void operator+=(const RandProj& rp);

    inline bool operator  == (const RandProj& rp)const
    {
        for (uint32_t i=0; i<proj_.length(); ++i)
            if (proj_.at(i)!= rp.at(i))
                return false;
        return true;
    }


    inline void save(FILE* f)
    {
        proj_.save(f);
    }

    inline void load(FILE* f)
    {
        proj_.load(f);
    }

    /**
     * @brief get number of projection
     *
     * static members
     * @return sizeof projection
     */
    inline int num_dimensions() const
    {
        return proj_.length();
    }
    /**
     * @brief Generates a CBitArray object from a random projection.
     *
     * @param projection source
     * @param bitArray target
     */
    void generate_bitarray(izenelib::util::CBitArray& bitArray);

    friend std::ostream& operator << (std::ostream& os, const RandProj& rj)
    {
        os<<rj.proj_;
        return os;
    }
};

}

#endif
