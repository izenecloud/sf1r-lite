#include "GroupMiningTask.h"
#include "DateStrParser.h"
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/util/FSUtil.hpp>
#include <mining-manager/MiningException.hpp>
#include <iostream>
#include <cassert>

using namespace sf1r::faceted;
	namespace
	{
	const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
	}