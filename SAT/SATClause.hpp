/**
 * @file SATClause.hpp
 * Defines class SATClause.
 */


#ifndef __SATClause__
#define __SATClause__

#include <string>
#include <iosfwd>

#include "Forwards.hpp"

#include "Lib/Allocator.hpp"
#include "Lib/InverseLookup.hpp"
#include "Lib/List.hpp"
#include "Lib/Metaiterators.hpp"
#include "Lib/Reflection.hpp"
#include "Lib/VirtualIterator.hpp"

#include "SATLiteral.hpp"

namespace SAT {

using namespace std;
using namespace Lib;
using namespace Kernel;

/**
 * Class to represent clauses.
 * @since 10/05/2007 Manchester
 */
class SATClause
{
public:
  DECL_ELEMENT_TYPE(SATLiteral);

  typedef ArrayishObjectIterator<SATClause> Iterator;

  DECL_ITERATOR_TYPE(Iterator);

  typedef double ActivityType;

  /** New clause */
  SATClause(unsigned length,bool kept=true)
    : _activity(0), _length(length), _kept(kept?1:0)
//      , _genCounter(0xFFFFFFFF)
  {}

  void* operator new(size_t,unsigned length);

  /**
   * Return the (reference to) the nth literal
   */
  inline SATLiteral& operator[] (int n)
  { return _literals[n]; }
  /** Return the (reference to) the nth literal */
  inline const SATLiteral& operator[] (int n) const
  { return const_cast<const SATLiteral&>(_literals[n]); }

  /** Return the length (number of literals) */
  inline unsigned length() const { return _length; }
  /** Alternative name for length to conform with other containers */
  inline unsigned size() const { return _length; }

  inline bool kept() const { return _kept; }
  inline void makeKept() { _kept=true; }
  inline void setKept(bool kept) { _kept=kept; }

//  inline unsigned conflicts() const { return _conflicts; }
//  inline void incConflicts() { _conflicts++; }

//  void resetSATAlgorithmData() {
//    _genCounter=0;
//    _conflicts=0;
//  }

//  unsigned getGenCounter() { ASS_NEQ(_genCounter, 0xFFFFFFFF); return _genCounter; }
//  void incGenCounter() { ASS_NEQ(_genCounter, 0xFFFFFFFF); _genCounter++; }

  /** Return a pointer to the array of literals. */
  inline SATLiteral* literals() { return _literals; }

  /** True if the clause is empty */
  inline bool isEmpty() const { return _length == 0; }

  ActivityType& activity() { return _activity; }


  void sort();


  void destroy();

  string toString() const;
  string toDIMACSString() const;

  /**
   * A numbering of literals for conversion of ground Clause objects into
   * SATClause objects.
   *
   * Positive literals are assigned positive numbers, and
   * negative ones assigned negative numbers.
   *
   * For each negative literal numbered as @b -n, the map must contain
   * also its positive counterpart numbered as @b n.
   */
  struct NamingContext {
    NamingContext() : nextVar(1) {}

    DHMap<Literal*, int> map;
    unsigned nextVar;
  };
  static SATClauseList* fromFOClauses(ClauseIterator clauses);
  static SATClauseList* fromFOClauses(NamingContext& context, ClauseIterator clauses);
  static SATClause* fromFOClause(NamingContext& context, Clause* clause);

  static SATClause* fromStack(SATLiteralStack& stack);

protected:
  static SATLiteral litToSAT(NamingContext& context, Literal* lit);

  ActivityType _activity;

  /** number of literals */
  unsigned _length : 31;

  unsigned _kept : 1;
//  unsigned _genCounter;

  /** Array of literals of this unit */
  SATLiteral _literals[1];
}; // class SATClause

};

std::ostream& operator<< (std::ostream& out, const SAT::SATClause& cl );

#endif /* __SATClause__ */
