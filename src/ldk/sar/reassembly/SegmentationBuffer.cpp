/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2014
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <WNS/ldk/sar/reassembly/SegmentationBuffer.hpp>
#include <WNS/ldk/arq/SelectiveRepeatIOD.hpp>

using namespace wns::ldk::sar::reassembly;

SegmentationBuffer::SegmentationBuffer(logger::Logger logger,
                                       int windowSize,
                                       int sequenceNumberSize,
                                       bool enableRetransmissions):
    lastSN_(0),
    logger_(logger),
    windowSize_(windowSize),
    sequenceNumberSize_(sequenceNumberSize),
    enableRetransmissions_(enableRetransmissions)
{
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  it is necessary to
 *
 * @Param cr
 */
/* ----------------------------------------------------------------------------*/
void SegmentationBuffer::initialize(wns::ldk::CommandReaderInterface* cr)
{
  assure(cr != NULL, "Invalid command reader given in SegmentationBuffer");

  commandReader_ = cr;

  assure(integrityCheck(), "Invalid state after initialization in SegmentationBuffer");
}

bool SegmentationBuffer::integrityCheck()
{
  if (commandReader_ == NULL)
    return false;

  return true;
}

connection
SegmentationBuffer::connectToReassemblySignal(const reassemblySlot_t& slot)
{
  return reassemble_.connect(slot);
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  uses the commandReader that was initialized to retrieve the command
 * for a given compound
 *
 * @Param c
 *
 * @Returns   CompoundPtr
 */
/* ----------------------------------------------------------------------------*/
SelectiveRepeatIODCommand* SegmentationBuffer::readCommand(const CompoundPtr& c)
{
  SelectiveRepeatIODCommand* command = NULL;
  command = commandReader_->readCommand<SelectiveRepeatIODCommand>(
      c->getCommandPool());
  assure(command!=NULL, "Command could not be retrieved");
  return command;
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis
 *
 * TODO: this function looks like it might be leaking
 *
 * TODO: right now this is horribly inefficient, we do a readcommand for every
 * item in the list twice
 * we're also using two lookups in the hash table when it could be one
 *
 * @Param compound
 * @Param timestamp
 */
/* ----------------------------------------------------------------------------*/
void SegmentationBuffer::push(CompoundPtr compound, timestamp_s timestamp)
{
  SelectiveRepeatIODCommand *command = readCommand(compound);

  // if the command is not segmented immediately pass the compound to the
  // upper layer, this should be dealt with outside of the segmentation buffer
  assert(command->isSegmented());

  if(enableRetransmissions_) {
    if(command->localTransmissionCounter > 0){
      MESSAGE_BEGIN(NORMAL, logger_, m, "missing pdu received: ");
      m << command->localTransmissionCounter;
      m << " index: " << genIndex(command);
      m << " " << command->groupId().time << command->groupId().clock ;
      m << " SN " << command->getSequenceNumber() << " index " << genIndex(command);
      MESSAGE_END();
    }
    // if the gap between the last received sn is bigger than 1
    // we're missing some packets
    // packet 1
    // lastSN := 1
    // packet 3
    // lastSN := 3
    // gap = 2
    // packet 2 is now received
    // if status was already sent, ignore it
    // if status was not sent, remove it from the missing pdu list
    int gap = command->bigSN() -  lastSN_;
    GroupNumber groupId = command->groupId();

    removeFromMissing(command->groupId(), command->bigSN());
    if(gap > 1) {
      // add missing pdu list
      for (unsigned int i = 1; i < gap; i++) {
        command->bigSN();
        appendMissing(command->groupId(), lastSN_+i);
        // lastSN_+i;
      }
    }

    //TODO: remove received packets from missingPdu List

    //TODO: remove old packets from missing pdu list
    // removeOldMissing();

    // if big sequence number small than last sequenceNumber
    // don't update lastSN
    if(command->bigSN() > lastSN_) {
      lastSN_ = command->bigSN();
    }
  }

  hashTable_[timestamp][this->genIndex(command)] = compound;

  if(checkCompleteness(hashTable_[timestamp])) {
    appendCompleted(timestamp);
    hashTable_.erase(timestamp);
  }
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  This method checks whether a compoundlist is completed
 *
 * @Param compoundList
 */
/* ----------------------------------------------------------------------------*/
bool SegmentationBuffer::checkCompleteness(const compoundReassembly_t& compoundList)
{
  bool isComplete = false;
  SelectiveRepeatIODCommand *command;
  SequenceNumber lastIndex;
  compoundReassembly_t::const_iterator it = compoundList.begin();
  CompoundPtr compound = it->second;
  int length = compoundList.size();
  for (int i = 0; it != compoundList.end(); ++it, i++)
  {
    command = readCommand(it->second);

    // first packet must have begin flag set
    if (i == 0) {
      if (!command->getBeginFlag()){
        break;
      }

      lastIndex = it->first;
      continue;
    }

    // check if there is a missing segment
    if (lastIndex != (it->first - 1)){
      break;
    }

    // last packet must have the end flag set if it's set the segment is complete
    if (i == length-1 && command->getEndFlag()){
      MESSAGE_SINGLE(VERBOSE, logger_, "Successfully completed segment");
      reassemble_(compoundList);
      return true;
    }

    lastIndex = it->first;
  }
  return isComplete;
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  magically sort all sequence numbers, the sequenceNumbers can't
 * get bigger than the amount of sequenceNumbers we have. it's almost an ordered
 * circular buffer
 *
 * @Param command
 *
 * @Returns   an index for the sequence number lookup table
 */
/* ----------------------------------------------------------------------------*/
SequenceNumber SegmentationBuffer::genIndex(SelectiveRepeatIODCommand* command){
  SequenceNumber index;

  if(command->getSequenceNumber() < command->getStartSN()){
    index = command->getSequenceNumber() + sequenceNumberSize_ - command->getStartSN();
  }
  else {
    index = command->getSequenceNumber() - command->getStartSN();
  }

  return index;
}

bool SegmentationBuffer::getMissing(SelectiveRepeatIODCommand* command){
    bool hasMissing = false;

    if(!enableRetransmissions_) {
      return false;
    }

    MESSAGE_BEGIN(NORMAL, logger_, m, "missing size: ");
    m << completedList_.size() << " " << missingPduList_.size();
    MESSAGE_END();
    command->addCompletedPdus(completedList_);
    command->addMissingPdus(missingPduList_);

   return hasMissing;
}

void SegmentationBuffer::removeFromMissing(GroupNumber groupId, BigSequenceNumber pduNum) {
  mapMissingPdu_t::const_iterator it = missingPduList_.find(groupId);
  if(it != missingPduList_.end()) {
    missingPduList_[groupId].remove(pduNum);
  }
}

void SegmentationBuffer::appendMissing(GroupNumber groupId, BigSequenceNumber pduNum) {
  MESSAGE_BEGIN(NORMAL, logger_, m, "missing added: ");
  m << groupId.time << groupId.clock ;
  MESSAGE_END();
  missingPduList_[groupId].push_back(pduNum);
  missingPduList_[groupId].unique();
}

void SegmentationBuffer::appendCompleted(GroupNumber groupId) {
  MESSAGE_BEGIN(NORMAL, logger_, m, "completed: ");
  m << groupId.time << groupId.clock ;
  MESSAGE_END();
  missingPduList_.erase(groupId);
  completedList_.push_back(groupId);
}
