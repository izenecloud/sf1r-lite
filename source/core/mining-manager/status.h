#ifndef STATUS_H
#define STATUS_H

#include <boost/thread/shared_mutex.hpp>
#include <boost/filesystem.hpp>

#include <common/Status.h>

namespace sf1r
{

enum MIA_STATUS
{
	NONE = 0, RAW_MAP = 1 << 0, FORWARD_INDEX = 1 << 1, DUPD = 1 << 2, /**< duplicated detection */
	TG = 1 << 3, /**< taxonomy generation */
	QR = 1 << 4, /**< query recommendation */
	SIM = 1 << 5,
	DC=1<<6,
	ISE=1 << 7
/**< similarity detection */
};

class MiningStatus : public Status
{
	int status_;
	boost::shared_mutex mutex_;
//	typedef boost::mutex::scoped_lock scoped_lock;
	std::string file_;

public:
	inline MiningStatus(const std::string& file) :
		status_(NONE), file_(file)
	{
		if (!boost::filesystem::exists(file))
		{
			save();
		}
		else
		{
			load();
		}
	}

	inline void save()
	{
		// do not save running and lastModified
		std::fstream status_stream(file_.c_str(), ios::out);
		status_stream << status_;
		status_stream.close();
	}

	inline void del()
	{
		boost::filesystem::remove(file_);
	}

	inline void load()
	{
		// do not load running and lastModified
		std::fstream status_stream(file_.c_str(), ios::in);
		status_stream >> status_;
		status_stream.close();
	}

	inline void set_raw_map()
	{
	    mutex_.lock_shared();

		status_ |= RAW_MAP;
		save();
		mutex_.unlock_shared();
	}

	inline bool get_raw_map()
	{
	    mutex_.lock_shared();

	    bool ret=status_ & RAW_MAP;
	    mutex_.unlock_shared();
		return ret;
	}

	inline void reset_raw_map()
	{
	    mutex_.lock_shared();
		status_ &= RAW_MAP ^ 0xffff;
		save();
		mutex_.unlock_shared();
	}

	inline void set_forward_index()
	{
	    mutex_.lock_shared();
		status_ |= FORWARD_INDEX;
		save();
		mutex_.unlock_shared();
	}

	inline bool get_forward_index()
	{
	    mutex_.lock_shared();
		bool ret=status_ & FORWARD_INDEX;
		mutex_.unlock_shared();
		return ret;
	}

	inline void reset_forward_index()
	{
	    mutex_.lock_shared();
		status_ &= FORWARD_INDEX ^ 0xffff;
		save();
		mutex_.unlock_shared();
	}

	inline void set_dupd()
	{
	    mutex_.lock_shared();
		status_ |= DUPD;
		save();
		mutex_.unlock_shared();
	}

	inline bool get_dupd()
	{
	    mutex_.lock_shared();
	    bool ret=status_ & DUPD;
	    mutex_.unlock_shared();
		return ret;
	}

	inline void reset_dupd()
	{
	    mutex_.lock_shared();
		status_ &= DUPD ^ 0xffff;
		mutex_.unlock_shared();
	}

	inline void set_tg()
	{
	    mutex_.lock_shared();
		status_ |= TG;
		save();
		mutex_.unlock_shared();
	}

	inline bool get_tg()
	{
	    mutex_.lock_shared();
		bool ret= status_ & TG;
		mutex_.unlock_shared();
		return ret;
	}

	inline void reset_tg()
	{
	    mutex_.lock_shared();
		status_ &= TG ^ 0xffff;
		save();
		mutex_.unlock_shared();
	}

	inline void set_qr()
	{
	    mutex_.lock_shared();
		status_ |= QR;
		save();
		mutex_.unlock_shared();
	}

	inline bool get_qr()
	{
	    mutex_.lock_shared();
	    bool ret=status_ & QR;
	    mutex_.unlock_shared();
		return ret;
	}

     inline void set_dc()
     {
         mutex_.lock_shared();
         status_ |= DC;
         save();
         mutex_.unlock_shared();
     }

     inline bool get_dc()
     {
         mutex_.lock_shared();
         bool ret=status_ & DC;
         mutex_.unlock_shared();
         return ret;
     }

     inline void set_ise()
     {
         mutex_.lock_shared();
         status_ |= ISE;
         save();
         mutex_.unlock_shared();
     }

     inline bool get_ise()
     {
         mutex_.lock_shared();
         bool ret=status_ & ISE;
         mutex_.unlock_shared();
         return ret;
     }

	inline void reset_qr()
	{
	    mutex_.lock_shared();
		status_ &= QR ^ 0xffff;
		save();
		mutex_.unlock_shared();
	}

	inline void set_sim()
	{
	    mutex_.lock_shared();
		status_ |= SIM;
		save();
		mutex_.unlock_shared();
	}

	inline bool get_sim()
	{
	    mutex_.lock_shared();
		bool ret=status_ & SIM;
		mutex_.unlock_shared();
		return ret;
	}

	inline void reset_sim()
	{
	    mutex_.lock_shared();
		status_ &= SIM ^ 0xffff;
		save();
		mutex_.unlock_shared();
	}

};

}

#endif
