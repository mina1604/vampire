/**
 * @file BackwardSubsumptionResolution.hpp
 * Defines class BackwardSubsumptionResolution.
 */


#ifndef __BackwardSubsumptionResolution__
#define __BackwardSubsumptionResolution__

#include "InferenceEngine.hpp"

namespace Inferences {

using namespace Indexing;

class BackwardSubsumptionResolution
: public BackwardSimplificationEngine
{
public:
  void attach(SaturationAlgorithm* salg);
  void detach();

  void perform(Clause* premise, BwSimplificationRecordIterator& simplifications);
private:
  struct ClauseExtractorFn;
  struct ClauseToBwSimplRecordFn;

  SimplifyingLiteralIndex* _index;
};

};

#endif /* __BackwardSubsumptionResolution__ */
