#ifndef _WNS_LDK_SAR_SEGMENTATIONBUFFER_H
#define _WNS_LDK_SAR_SEGMENTATIONBUFFER_H

#include <WNS/ldk/Compound.hpp>
#include <WNS/ldk/CommandReaderInterface.hpp>
#include <WNS/ldk/arq/SelectiveRepeatIODCommand.hpp>
#include <deque>
#include <ctime>

using namespace std;
using namespace wns::ldk;
using namespace wns::ldk::arq;
using namespace boost::signals;

namespace wns { namespace ldk { namespace sar { namespace reassembly {

typedef boost::signal<bool (const compoundReassembly_t&)> reassemblySignal_t;
typedef reassemblySignal_t::slot_type reassemblySlot_t;

class SegmentationBuffer
{
public:
  SegmentationBuffer (logger::Logger logger,
                      int windowSize,
                      int sequenceNumberSize,
                      bool enableRetransmissions);
  void initialize(CommandReaderInterface*);
  void updateSenderQueue();
  void push(CompoundPtr compound, timestamp_s timestamp);
  connection connectToReassemblySignal(const reassemblySlot_t& slot);
  bool getMissing(SelectiveRepeatIODCommand*);

private:
  bool checkCompleteness (const compoundReassembly_t&);
  bool integrityCheck();
  SelectiveRepeatIODCommand* readCommand(const wns::ldk::CompoundPtr&);
  SequenceNumber genIndex(SelectiveRepeatIODCommand*);
  void appendMissing(GroupNumber, BigSequenceNumber);
  void removeFromMissing(GroupNumber, BigSequenceNumber);
  void appendCompleted(GroupNumber);
  //TODO: make sure we get the right windowSize from the inmarsat spec
  int windowSize_;

  int sequenceNumberSize_;

  bool enableRetransmissions_;

  BigSequenceNumber lastSN_;

  logger::Logger logger_;

  compoundHashTable_t hashTable_;

  CommandReaderInterface* commandReader_;

  reassemblySignal_t reassemble_;

  completedList_t completedList_;

  mapMissingPdu_t missingPduList_;
};

}}}}

#endif /* end of include guard: __WNS_LDK_SAR_SEGMENTATIONBUFFER_H */
