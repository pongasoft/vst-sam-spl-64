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

#ifndef VST_SAM_SPL_64_UNDOHISTORY_H
#define VST_SAM_SPL_64_UNDOHISTORY_H

#include <forward_list>

namespace pongasoft::VST::SampleSplitter::GUI {

/**
 * Encapsulates an action on `CurrentSample` as an object (in order to be able to handle undo/redo) */
struct SampleAction
{
  enum class Type { kCut, kCrop, kTrim, kNormalize0, kNormalize3, kNormalize6, kResample, kLoad, kSample };

  explicit SampleAction(Type iType) : fType{iType} {}

  Type fType;
  NumSlice fNumSlices{}; // used in undo
  SampleRange fSelectedSampleRange{-1}; // used with kCut/kCrop and undo
  Percent fOffsetPercent{}; // used in undo
  Percent fZoomPercent{}; // used in undo
  UTF8Path fFilePath{}; // used with kLoad
  SharedSampleBuffersVersion fRTVersion; // used with kSample
};

class UndoHistory
{
public:
  /**
   * These objects are saved in the undo history in order to be replayed: `fAction` applied on `fFile` provides
   * the new CurrentSample
   */
  struct Entry
  {
    Entry(SampleAction iAction, CurrentSample const &iSample, SampleFile iFile) :
      fAction{std::move(iAction)},
      fSource{iSample.getSource()},
      fUpdateType{iSample.getUpdateType()},
      fFile(std::move(iFile))
    {}

    SampleAction fAction;
    CurrentSample::Source fSource;
    CurrentSample::UpdateType fUpdateType;
    SampleFile fFile;
  };

public:
  void addEntry(SampleAction iAction, CurrentSample const &iSample, SampleFile iFile)
  {
    UndoHistory::Entry undoEntry{iAction, iSample, std::move(iFile)};
    fUndoHistory.push_front(undoEntry);
  }

  void clearRedoHistory() { fRedoHistory.clear(); }

  Entry undo()
  {
    DCHECK_F(hasUndoHistory());
    auto lastExecutedAction = fUndoHistory.front();
    fUndoHistory.pop_front();
    fRedoHistory.push_front(lastExecutedAction.fAction);
    return lastExecutedAction;
  }

  SampleAction redo()
  {
    DCHECK_F(hasRedoHistory());
    auto lastAction = fRedoHistory.front();
    fRedoHistory.pop_front();
    return lastAction;
  }

  // getLastUndoEntry
  inline Entry const *getLastUndoEntry() const {
    return fUndoHistory.empty() ? nullptr : &fUndoHistory.front();
  }

  /**
   * Clears the entire undo/redo history
   *
   * @return `true` if there was anything to clear */
  bool clearActionHistory()
  {
    if(hasActionHistory())
    {
      fUndoHistory.clear();
      fRedoHistory.clear();
      return true;
    }
    return false;
  }


  // hasUndoHistory
  inline bool hasUndoHistory() const { return !fUndoHistory.empty(); }
  // hasRedoHistory
  inline bool hasRedoHistory() const { return !fRedoHistory.empty(); }
  // hasActionHistory
  inline bool hasActionHistory() const { return hasUndoHistory() || hasRedoHistory(); }

private:
  std::forward_list<Entry> fUndoHistory{};
  std::forward_list<SampleAction> fRedoHistory{};

};

}

#endif //VST_SAM_SPL_64_UNDOHISTORY_H
