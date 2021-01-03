/*
 * Copyright (c) 2020 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#ifndef VST_SAM_SPL_64_SHAREDOBJECTMGR_H
#define VST_SAM_SPL_64_SHAREDOBJECTMGR_H

#include <pluginterfaces/vst/vsttypes.h>

#include <pongasoft/Utils/Concurrent/SpinLock.h>

#include "SampleBuffers.h"

namespace pongasoft::VST::SampleSplitter {

/**
 * This class is meant to **share** an object (of type `ObjectType`) between the UI and the RT. Every method is prefixed
 * by which side should call it. So `uiXXX` should be called from the UI side and `rtXXX` should be called from
 * the RT side only. This class is not intended to be used frequently but only in very specific cases where sharing
 * large amounts of data via messaging is prohibitive.
 *
 * Example of usage from RT:
 *
 * - RT has a new `ObjectType` to share with UI
 * - RT calls `mgr.rtSetObject` and gets a version [v]
 * - RT sends `v` to UI via messaging (`RTJmbOutParam.broadcast(v)` with Jamba)
 * - UI receives `v` via messaging (simple callback with Jamba)
 * - UI calls `mgr.uiAdjustObjectFromRT(v)` and gets the new `ObjectType`
 *
 * Usage from UI follows the exact same pattern
 *
 * - UI has a new `ObjectType` to share with RT
 * - UI calls `mgr.guiSetObject` and gets a version [v]
 * - UI sends `v` to RT via messaging (`GUIJmbParam.broadcast(v)` with Jamba)
 * - RT receives `v` via messaging (`RTJmbInParam.pop()` with Jamba)
 * - RT calls `mgr.rtAdjustObjectFromUI(v)` and gets the new `ObjectType` (recommended to call
 *   `mgr.rtAdjustObjectFromUI(v).get()`)
 *
 * In order to share an instance of this class between RT and UI, it is recommended to have RT creates the instance
 * and share it with UI using messaging (see `PointerSerializer`).
 *
 * @note 1. Although this is not enforced, it is **strongly** recommended that the object `ObjectType` is
 *          immutable and never directly modified in the RT or the UI since it is actually shared (unless you use
 *          some locks to do so which is not recommended either). In other words,
 *          `uiGetObject()->functionWhichMutateObject()` should never be called.
 *          Instead, create a new object and use `SharedObjectMgr::uiSetObject` (resp. `SharedObjectMgr::rtSetObject`).
 *
 * @note 2. This class relies on the fact that `std::shared_ptr` is thread safe **only** in regards to the counter which
 *          is why it uses a `SpinLock` when it gets assigned (as well as deal with version) to guarantee that it is
 *          thread safe.
 *
 * @note 3. Since `std::shared_ptr` is thread safe (from the counter point of view only!), it is technically
 *          possible to use it directly in RT, but it is not recommended. It is better to use the underlying pointer
 *          directly. As long as the class is used as intended (meaning `rtXXX` methods are **only** called from
 *          RT), the underlying pointer remains valid until `rtAdjustObjectFromUI` is called again so you are in
 *          control.
 */
template<typename ObjectType, typename VersionType = int64>
class SharedObjectMgr
{
public:
  using object_type = ObjectType;
  using version_type = VersionType;
  
  /**
   * Shortcut to return the shared object from the UI side.
   *
   * @note Must be called from the UI side **only** */
  inline std::shared_ptr<ObjectType> uiGetObject() const
  {
    auto lock = fLock.acquire();
    return fUIObject;
  }

  /**
   * Sets the new object from the UI side and returns the version that needs to be communicated to RT
   *
   * @note Must be called from the UI side **only** */
  inline VersionType uiSetObject(std::shared_ptr<ObjectType> iObject)
  {
    std::shared_ptr<ObjectType> objectToDelete{};
    VersionType res{};

    // CRITICAL SECTION START
    {
      auto lock = fLock.acquire();
      objectToDelete = std::move(fUIObject);
      fUIObject = std::move(iObject);
      fUIVersion = ++fVersion;
      res = fUIVersion;
//    DLOG_F(INFO, "std::shared_ptrMgr::uiSetObject -> %lld", fUIVersion);
    }
    // CRITICAL SECTION EMD

    // ensures that if object needs to be deleted, it is deleted OUTSIDE the lock
    return res;
  }

  /**
   * Called after RT changes the object and communicates the version to UI. Note that this method returns `nullptr`
   * in the event that the version is outdated (or simply invalid).
   *
   * @param iRTVersion The version sent by RT
   * @param oUpdated Since setting the object to `nullptr` could also be valid, this parameter can be provided to
   *                 be able to distinguish between a valid `nullptr` new object and an invalid/outdated version.
   *                 If your code never sets the object to `nullptr` then it can be ignored.
   *
   * @note Must be called from the UI side **only** */
  std::shared_ptr<ObjectType> uiAdjustObjectFromRT(VersionType iRTVersion, bool *oUpdated = nullptr)
  {
    std::shared_ptr<ObjectType> objectToDelete{};
    std::shared_ptr<ObjectType> res{};

    // CRITICAL SECTION START
    {
      auto lock = fLock.acquire();
//    DLOG_F(INFO, "std::shared_ptrMgr::uiAdjustObjectFromRT(%lld)", iRTVersion);
      if(iRTVersion > fUIVersion && iRTVersion <= fVersion)
      {
        objectToDelete = std::move(fUIObject);
        fUIObject = fRTObject;
        fUIVersion = fRTVersion;
        if(oUpdated)
          *oUpdated = true;
        res = fUIObject;
      }
      else
      {
        if(oUpdated)
          *oUpdated = false;
      }
    }
    // CRITICAL SECTION EMD

    // ensures that if object needs to be deleted, it is deleted OUTSIDE the lock
    return res;
  }

  /**
   * Shortcut to return the shared object from the RT side.
   *
   * @note Must be called from the RT side **only** */
  inline std::shared_ptr<ObjectType> rtGetObject() const
  {
    auto lock = fLock.acquire();
    return fRTObject;
  }

  /**
   * Sets the new object from the RT side and returns the version that needs to be communicated to UI
   *
   * @note Must be called from the RT side **only** */
  inline VersionType rtSetObject(std::shared_ptr<ObjectType> iObject)
  {
    auto lock = fLock.acquire();
    fRTObject = std::move(iObject);
    fRTVersion = ++fVersion;
//    DLOG_F(INFO, "std::shared_ptrMgr::rtSetObject -> %lld", fRTVersion);
    return fRTVersion;
  }

  /**
   * Called after UI changes the object and communicates the version to RT. Note that this method returns `nullptr`
   * in the event that the version is outdated (or simply invalid).
   *
   * @param iUIVersion The version sent by UI
   * @param oUpdated Since setting the object to `nullptr` could also be valid, this parameter can be provided to
   *                 be able to distinguish between a valid `nullptr` new object and an invalid/outdated version.
   *                 If your code never sets the object to `nullptr` then it can be ignored.
   *
   * @note Must be called from the RT side **only** */
  inline std::shared_ptr<ObjectType> rtAdjustObjectFromUI(VersionType iUIVersion, bool *oUpdated = nullptr)
  {
    auto lock = fLock.acquire();
//    DLOG_F(INFO, "std::shared_ptrMgr::rtAdjustObjectFromUI(%lld)", iUIVersion);
    if(iUIVersion > fRTVersion && iUIVersion <= fVersion)
    {
      fRTObject = fUIObject;
      fRTVersion = fUIVersion;
      if(oUpdated)
        *oUpdated = true;
      return fRTObject;
    }
    if(oUpdated)
      *oUpdated = false;
    return nullptr;
  }

private:
  mutable SpinLock fLock{};

  version_type fVersion{};

  std::shared_ptr<ObjectType> fUIObject{};
  version_type fUIVersion{};

  std::shared_ptr<ObjectType> fRTObject{};
  version_type fRTVersion{};
};


}

#endif //VST_SAM_SPL_64_SHAREDOBJECTMGR_H
