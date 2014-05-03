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
void SegmentationBuffer::push(CompoundPtr compound)
{
  SelectiveRepeatIODCommand *command = readCommand(compound);

  // if the command is not segmented immediately pass the compound to the
  // upper layer, this should be dealt with outside of the segmentation buffer
  if(enableRetransmissions_) {
    if(command->localTransmissionCounter > 0){
      MESSAGE_BEGIN(NORMAL, logger_, m, "missing pdu received: ");
      m << command->localTransmissionCounter;
      m << " SN " << command->getSequenceNumber();
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

    removeFromMissing(command->bigSN());
    if(gap > 1) {
      MESSAGE_BEGIN(NORMAL, logger_, m, "gap length: ");
      m << gap;
      MESSAGE_END();
      // add missing pdu list
      for (unsigned int i = 1; i < gap; i++) {
        appendMissing(lastSN_+i);
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
    // only packets that are received out of order can be retransmissions
    else if (isCompleted(command)) {
      // discard the packet if it wasn't missing
      MESSAGE_BEGIN(NORMAL, logger_, m, "check missing, duplicate received: ");
      m << command->bigSN();
      MESSAGE_END();
      return;
    }
  }

  lSegmentCompound_t sdus = command->baseSDUs();
  lSegmentCompound_t::iterator it = sdus.begin();

  for (; it != sdus.end(); ++it)
  {
    hashTable_[it->timestamp][this->genIndex(command->getSequenceNumber(),
                                         it->startSN)] = compound;

    if(checkCompleteness(hashTable_[it->timestamp],
                         it->sdu,
                         it->startSN,
                         it->endSN)) {
      appendCompleted(it->timestamp);
      hashTable_.erase(it->timestamp);
    }
  }

}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  This method checks whether a compoundlist is completed
 *
 * @Param compoundList
 */
/* ----------------------------------------------------------------------------*/
bool SegmentationBuffer::checkCompleteness(const compoundReassembly_t& compoundList,
                                           CompoundPtr sdu,
                                           SequenceNumber startSN,
                                           SequenceNumber endSN)
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
      if (command->getSequenceNumber()!=startSN){
        break;
      }

      // not segmented, it's both begin, and end flag
      if (startSN == endSN){
        MESSAGE_SINGLE(VERBOSE, logger_, "Successfully completed segment");
        reassemble_(sdu);
        return true;
      }

      lastIndex = it->first;
      continue;
    }

    // check if there is a missing segment
    if (lastIndex != (it->first - 1)){
      break;
    }

    // last packet must have the end flag set if it's set the segment is complete
    if (i == length-1 && command->getSequenceNumber() == endSN){
      MESSAGE_SINGLE(VERBOSE, logger_, "Successfully completed segment");
      reassemble_(sdu);
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
SequenceNumber SegmentationBuffer::genIndex(SequenceNumber sn, SequenceNumber startSN){
  SequenceNumber index;

  if(sn < startSN){
    index = sn + sequenceNumberSize_ - startSN;
  }
  else {
    index = sn - startSN;
  }

  return index;
}

bool SegmentationBuffer::getMissing(SelectiveRepeatIODCommand* command){
    bool hasMissing = false;

    if(!enableRetransmissions_) {
      return false;
    }

    MESSAGE_BEGIN(NORMAL, logger_, m, "completed size: ");
    m << completedList_.size() << " missing size: " << missingPduList_.size();
    MESSAGE_END();
    command->addCompletedPdus(completedList_);
    command->addMissingPdus(missingPduList_);

   return hasMissing;
}

void SegmentationBuffer::removeFromMissing(BigSequenceNumber pduNum) {
  missingPduList_.remove(pduNum);
}

void SegmentationBuffer::appendMissing(BigSequenceNumber pduNum) {
  MESSAGE_SINGLE(NORMAL, logger_, "missing added: ");
  missingPduList_.push_back(pduNum);
  missingPduList_.unique();
}

void SegmentationBuffer::appendCompleted(GroupNumber groupId) {
  MESSAGE_BEGIN(NORMAL, logger_, m, "completed: ");
  m << groupId;
  MESSAGE_END();
  completedList_.insert(groupId);
}


bool SegmentationBuffer::isCompleted(const SelectiveRepeatIODCommand *command) {

  return false;
}
