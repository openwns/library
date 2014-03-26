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
                                       int sequenceNumberSize):
    logger_(logger),
    windowSize_(windowSize),
    sequenceNumberSize_(sequenceNumberSize)
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

  // create compoundcontainer if it doesn't exist
  if (hashTable_.find(timestamp) == hashTable_.end()){
    hashTable_[timestamp] = compoundReassembly_t();
  }
  compoundReassembly_t compoundList = hashTable_[timestamp];

  compoundList[this->genIndex(command)] = compound;
  hashTable_[timestamp] = compoundList;

  checkCompleteness(compoundList);
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  This method checks whether a compoundlist is completed
 *
 * @Param compoundList
 */
/* ----------------------------------------------------------------------------*/
void SegmentationBuffer::checkCompleteness(compoundReassembly_t compoundList)
{
  SelectiveRepeatIODCommand *command;
  SequenceNumber lastSn;
  compoundReassembly_t::iterator it = compoundList.begin();
  int length = compoundList.size();
  for (int i = 0; it != compoundList.end(); ++it, i++)
  {
    command = readCommand(it->second);

    // first packet must have begin flag set
    if (i == 0) {
      if (!command->getBeginFlag()){
        return;
      }

      lastSn = command->getSequenceNumber();
      continue;
    }

    // check if there is a missing segment
    if (lastSn != (command->getSequenceNumber() - 1)){
      return;
    }

    // last packet must have the end flag set if it's set the segment is complete
    if (i == length-1 && command->getEndFlag()){
      MESSAGE_SINGLE(VERBOSE, logger_, "Successfully completed segment");
      reassemble_(compoundList);
    }

    lastSn = command->getSequenceNumber();
  }
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

void SegmentationBuffer::findMissing(const compoundReassembly_t& compoundList){

}
