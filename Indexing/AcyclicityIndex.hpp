/**
 * @file AcyclicityIndex.hpp
 * Defines class AcyclicityIndex
 */

#ifndef __AcyclicityIndex__
#define __AcyclicityIndex__

#include "Indexing/Index.hpp"

#include "Kernel/Clause.hpp"
#include "Kernel/Ordering.hpp"
#include "Kernel/Term.hpp"

#include "Indexing/TermIndexingStructure.hpp"

#include "Lib/DHMap.hpp"
#include "Lib/List.hpp"
#include "Lib/VirtualIterator.hpp"

#include "Forwards.hpp"

namespace Indexing {

struct CycleQueryResult {
  CycleQueryResult(Lib::List<Kernel::Literal*>* l,
                   Lib::List<Kernel::Clause*>* p,
                   Lib::List<Kernel::Clause*>* c)
    :
    literals(l),
    premises(p),
    clausesTheta(c)
  {}

  unsigned totalLengthClauses();
  
  Lib::List<Kernel::Literal*>* literals;
  Lib::List<Kernel::Clause*>* premises;
  Lib::List<Kernel::Clause*>* clausesTheta; // the three lists should be the same length
};

typedef Lib::VirtualIterator<CycleQueryResult*> CycleQueryResultsIterator;

class AcyclicityIndex
: public Index
{
public:
  AcyclicityIndex(Indexing::TermIndexingStructure* tis, Kernel::Ordering &ord) :
    _sIndexes(),
    _tis(tis),
    _ord(ord)
  {}

  ~AcyclicityIndex() {}
  
  void insert(Kernel::Literal *lit, Kernel::Clause *c);
  void remove(Kernel::Literal *lit, Kernel::Clause *c);

  CycleQueryResultsIterator queryCycles(Kernel::Literal *lit, Kernel::Clause *c);
             
  CLASS_NAME(AcyclicityIndex);
  USE_ALLOCATOR(AcyclicityIndex);
protected:
  void handleClause(Kernel::Clause* c, bool adding);
private:
  bool matchesPattern(Kernel::Literal *lit, Kernel::TermList *&fs, Kernel::TermList *&t, unsigned *sort);
  Lib::List<TermList*>* getSubterms(Kernel::Term *t);
  
  struct IndexEntry;
  struct CycleSearchTreeNode;
  struct CycleSearchIterator;
  typedef Lib::DHMap<Kernel::Literal*, IndexEntry*> SIndex;

  Lib::DHMap<unsigned, SIndex*> _sIndexes;
  Indexing::TermIndexingStructure* _tis;
  Kernel::Ordering &_ord;
};

}

#endif