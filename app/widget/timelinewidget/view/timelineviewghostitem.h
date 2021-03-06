/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef TIMELINEVIEWGHOSTITEM_H
#define TIMELINEVIEWGHOSTITEM_H

#include <QVariant>

#include "project/item/footage/footage.h"
#include "timeline/timelinecommon.h"
#include "timelineviewblockitem.h"
#include "timelineviewrect.h"

namespace olive {
/**
 * @brief A graphical representation of changes the user is making before they apply it
 */
class TimelineViewGhostItem
{
public:
  enum DataType {
    kAttachedBlock,
    kReferenceBlock,
    kAttachedFootage,
    kGhostIsSliding,
    kTrimIsARollEdit,
    kTrimShouldBeIgnored
  };

  TimelineViewGhostItem();

  static TimelineViewGhostItem* FromBlock(Block *block, const TrackReference &track);

  bool CanHaveZeroLength() const;

  bool GetCanMoveTracks() const;
  void SetCanMoveTracks(bool e);

  const rational& GetIn() const;
  const rational& GetOut() const;
  const rational& GetMediaIn() const;

  rational GetLength() const;
  rational GetAdjustedLength() const;

  void SetIn(const rational& in);
  void SetOut(const rational& out);
  void SetMediaIn(const rational& media_in);

  void SetInAdjustment(const rational& in_adj);
  void SetOutAdjustment(const rational& out_adj);
  void SetTrackAdjustment(const int& track_adj);
  void SetMediaInAdjustment(const rational& media_in_adj);

  const rational& GetInAdjustment() const;
  const rational& GetOutAdjustment() const;
  const rational& GetMediaInAdjustment() const;
  const int& GetTrackAdjustment() const;

  rational GetAdjustedIn() const;
  rational GetAdjustedOut() const;
  rational GetAdjustedMediaIn() const;
  TrackReference GetAdjustedTrack() const;

  const Timeline::MovementMode& GetMode() const;
  void SetMode(const Timeline::MovementMode& GetMode);

  bool HasBeenAdjusted() const;

  QVariant GetData(int key) const
  {
    return data_.value(key);
  }

  void SetData(int key, const QVariant& value)
  {
    data_.insert(key, value);
  }

  const TrackReference& GetTrack() const
  {
    return track_;
  }

  void SetTrack(const TrackReference& track)
  {
    track_ = track;
  }

  bool IsInvisible() const
  {
    return invisible_;
  }

  void SetInvisible(bool e)
  {
    invisible_ = e;
  }

protected:

private:
  rational in_;
  rational out_;
  rational media_in_;

  rational in_adj_;
  rational out_adj_;
  rational media_in_adj_;

  int track_adj_;

  Timeline::MovementMode mode_;

  bool can_have_zero_length_;
  bool can_move_tracks_;

  TrackReference track_;

  QHash<int, QVariant> data_;

  bool invisible_;

};

}

#endif // TIMELINEVIEWGHOSTITEM_H
