#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// protobuf wrapper convenience header
//
// The core protobuf types (CBasePB, CMsgQAngle, CMsgVector,
// CCSGOInterpolationInfoPB, CCSGOInputHistoryEntryPB, CCSGOUserCmdPB,
// CBaseUserCmdPB, RepeatedPtrField) are defined in datatypes/usercmd.h.
// This header re-exports them and provides additional utilities.
// ---------------------------------------------------------------
#include "../datatypes/usercmd.h"

// ---------------------------------------------------------------
// RepeatedPtrField_t<T> — template alias for Google protobuf
//                         repeated pointer field wrapper
// @note: this is identical to RepeatedPtrField<T> in usercmd.h
//        but provided as an alternate name for compatibility
// ---------------------------------------------------------------
template<typename T>
using RepeatedPtrField_t = RepeatedPtrField<T>;
