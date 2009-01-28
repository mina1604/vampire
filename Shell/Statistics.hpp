/**
 * @file Statistics.hpp
 * Defines proof-search statistics
 *
 * @since 02/01/2008 Manchester
 */

#ifndef __Statistics__
#define __Statistics__

namespace Kernel {
  class Unit;
}

namespace Shell {

/**
 * Class Statistics
 * @since 02/01/2008 Manchester
 */
class Statistics
{
public:
  Statistics();

  void print();

  // Input
  /** number of input clauses */
  unsigned inputClauses;
  /** number of input formulas */
  unsigned inputFormulas;

  // Preprocessing
  /** number of formula names introduced during preprocessing */
  unsigned formulaNames;
  /** number of initial clauses */
  unsigned initialClauses;

  //Generating inferences
  /** number of clauses generated by factoring*/
  unsigned factoring;
  /** number of clauses generated by binary resolution*/
  unsigned resolution;
  /** number of clauses generated by forward superposition*/
  unsigned forwardSuperposition;
  /** number of clauses generated by backward superposition*/
  unsigned backwardSuperposition;

  // Simplifying inferences
  /** number of duplicate literals deleted */
  unsigned duplicateLiterals;
  /** number of literals s |= s deleted */
  unsigned trivialInequalities;

  // Deletion inferences
  /** number of tautologies A \/ ~A */
  unsigned simpleTautologies;
  /** number of equational tautologies s=s */
  unsigned equationalTautologies;
  /** number of forward subsumed clauses */
  unsigned forwardSubsumed;
  /** number of backward subsumed clauses */
  unsigned backwardSubsumed;
  /** number of forward subsumption resolutions */
  unsigned forwardSubsumptionResolution;

  // Saturation
  /** all clauses ever occurring in the unprocessed queue */
  unsigned generatedClauses;
  /** all passive clauses */
  unsigned passiveClauses;
  /** all active clauses */
  unsigned activeClauses;

  /** termination reason */
  enum TerminationReason {
    /** refutation found */
    REFUTATION,
    /** satisfiability detected (saturated set built) */
    SATISFIABLE,
    /** saturation terminated but an incomplete strategy was used */
    UNKNOWN,
    /** time limit reached */
    TIME_LIMIT,
    /** memory limit reached */
    MEMORY_LIMIT
  };
  /** termination reason */
  TerminationReason terminationReason;
  /** refutation, if any */
  Kernel::Unit* refutation;
}; // class Statistics

}

#endif
