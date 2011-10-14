#ifndef CORE_INDEXING_PROGRESS_H
#define CORE_INDEXING_PROGRESS_H
/**
 * @file core/common/IndexingProgress.h
 * @date Created <2009-11-01 17:11:07>
 * @date Updated <2009-11-09 13:43:34>
 * @Author Peisheng Wang
 *
 */
#include <string>
#include <sstream>
#include <iostream>
#include <util/ClockTimer.h>

#include "Status.h"

namespace sf1r{
/**
 *@class IndexingProgress
 *@brief It is in charge of displaying indexing progress status, including
 *    indexing rate, indexing left time, indexing finished percentage.
 *
 */

struct IndexingProgress {
	int currentFileIdx;
	std::string currentFileName;
	int totalFileNum;

	long totalFileSize_;
	long totalFilePos_;

	long currentFilePos_;
	long currentFileSize_;
	izenelib::util::ClockTimer timer;

	IndexingProgress() {
		reset();
	}

	float getTotalPercent() {
		if (currentFileIdx > totalFileNum) {
			return 1.0f;
		} else if (totalFileNum == 1) {
			if (currentFileSize_ == 0)
				return 0.0f;
			else
				return (float) currentFilePos_ / (float) currentFileSize_;
		} else {
			if (totalFileSize_ == 0)
				return 0.0f;
			else
				return (float) totalFilePos_ / (float) totalFileSize_;
		}
	}

	float getElapsed() {
		return timer.elapsed();
	}

	float getLeft() {
		if (totalFilePos_ != 0) {
			return getElapsed() * (totalFileSize_ - totalFilePos_)
					/ totalFilePos_;
		} else {
			return 24 * 3600;
		}
	}

	void getIndexingStatus(Status& status) {
		int currentPercent = 0;
		int totalPercent = 0;
		if (currentFileSize_ != 0)
			currentPercent = 100 * currentFilePos_ / currentFileSize_;

		if (totalFileNum == 1)
			totalPercent = currentPercent;
		else if (totalFileSize_ != 0)
			totalPercent
					= (int) ((double) totalFilePos_ / totalFileSize_ * 100);

		float elapse = timer.elapsed();
		float timeLeft;
		int leftmin = 0;
		int leftsec = 0;
		if (totalFilePos_ != 0) {
			timeLeft = elapse * (totalFileSize_ - totalFilePos_)
					/ totalFilePos_;
			leftmin = int(timeLeft / 60);
			leftsec = int(timeLeft) - 60 * leftmin;
		}

		if (currentFileIdx > totalFileNum) {
			currentFileIdx = totalFileNum;
			timeLeft = leftmin = leftsec = 0;
			currentPercent = totalPercent = 100;
		}

		std::stringstream ss;
		ss << "  [" << currentFileIdx << "/" << totalFileNum << "]"
				<< currentFileName << "[" << currentPercent << "% - Total:"
				<< totalPercent << "%]";
		ss << "\t[left: " << leftmin << "m" << leftsec << "s]";

		std::cout << std::endl << ss.str() << std::flush;
		status.metaInfo_ = ss.str();
	}

	int getNumOfIndexedFiles() {
		return totalFileNum;
	}

	void reset() {
		currentFileIdx = 0;
		currentFileName = "";
		totalFileNum = 0;
		totalFileSize_ = totalFilePos_ = 0;
		currentFilePos_ = currentFileSize_ = 0;
		timer.restart();
	}
};

}
#endif // CORE_INDEXING_PROGRESS_H
