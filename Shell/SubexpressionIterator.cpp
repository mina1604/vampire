#include "Kernel/Formula.hpp"
#include "Kernel/Term.hpp"

#include "SubexpressionIterator.hpp"

namespace Shell {
  using namespace Lib;
  using namespace Kernel;

  SubexpressionIterator::SubexpressionIterator(Formula* f) {
    _subexpressions.push(new Expression(f));
  }
  SubexpressionIterator::SubexpressionIterator(FormulaList* fs) {
    FormulaList::Iterator fi(fs);
    while (fi.hasNext()) {
      Formula* f = fi.next();
      _subexpressions.push(new Expression(f));
    }
  }
  SubexpressionIterator::SubexpressionIterator(Term* t) {
    _subexpressions.push(new Expression(t));
  }
  SubexpressionIterator::SubexpressionIterator(TermList ts) {
    _subexpressions.push(new Expression(ts));
  }

  bool SubexpressionIterator::hasNext() {
    CALL("SubexpressionIterator::hasNext");
    return _subexpressions.isNonEmpty();
  }

  SubexpressionIterator::Expression* SubexpressionIterator::next() {
    CALL("SubexpressionIterator::next");

    ASS(hasNext());
    Expression* expression = _subexpressions.pop();
    int polarity = expression->_polarity;

    switch (expression->_tag) {
      case Expression::Tag::FORMULA: {
        Formula* f = expression->_formula;
        switch (f->connective()) {
          case LITERAL:
            /**
             * Note that we do not propagate the polarity here, because
             * formula-level if-then-else and let-in cannot occur inside literals
             */
            _subexpressions.push(new Expression(*f->literal()->args()));
            break;

          case AND:
          case OR: {
            FormulaList::Iterator args(f->args());
            while (args.hasNext()) {
              _subexpressions.push(new Expression(args.next(), polarity));
            }
            break;
          }

          case IMP:
            _subexpressions.push(new Expression(f->left(), -polarity));
            _subexpressions.push(new Expression(f->right(), polarity));
            break;

          case IFF:
          case XOR:
            _subexpressions.push(new Expression(f->left(),  0));
            _subexpressions.push(new Expression(f->right(), 0));
            break;

          case NOT:
            _subexpressions.push(new Expression(f->uarg(), -polarity));
            break;

          case FORALL:
          case EXISTS:
            _subexpressions.push(new Expression(f->qarg(), polarity));
            break;

          case BOOL_TERM:
            /**
             * Note, that here we propagate the polarity from the formula to
             * its underlying boolean term. This is the only place in the code
             * where the polarity of a term can be set to negative.
             */
            _subexpressions.push(new Expression(f->getBooleanTerm(), polarity));
            break;

          default:
            break;
        }
        break;
      }

      case Expression::Tag::TERM: {
        Term* term = expression->_term;
        if (term->isSpecial()) {
          Term::SpecialTermData* sd = term->getSpecialData();
          switch (sd->getType()) {
            case Term::SF_FORMULA:
              /**
               * Note that here we propagate the polarity of the boolean term to its
               * underlying formula
               */
              _subexpressions.push(new Expression(sd->getFormula(), polarity));
              break;

            case Term::SF_ITE:
              /**
               * Regardless of the polarity of the whole if-then-else expression,
               * the polarity of the condition is always 0. This is because you
               * can always see "$ite(C, A, B)" as "(C => A) && (~C => B)"
               */
              _subexpressions.push(new Expression(sd->getCondition(), 0));
              _subexpressions.push(new Expression(*term->nthArgument(0), polarity));
              _subexpressions.push(new Expression(*term->nthArgument(1), polarity));
              break;

            case Term::SF_LET:
            case Term::SF_LET_TUPLE:
              /**
               * The polarity of the body of let-bindings is 0.
               * An expression "$let(f := A, ...)", where A is a formula,
               * is semantically equivalent to f <=> A && ...
               */
              _subexpressions.push(new Expression(sd->getBinding(), 0));
              _subexpressions.push(new Expression(*term->nthArgument(0), polarity));
              break;

            case Term::SF_TUPLE:
              _subexpressions.push(new Expression(sd->getTupleTerm()));
              break;

#if VDEBUG
            default:
              ASSERTION_VIOLATION_REP(term->toString());
#endif
          }
        } else {
          Term::Iterator args(term);
          while (args.hasNext()) {
            _subexpressions.push(new Expression(args.next()));
          }
        }
        break;
      }

      case Expression::Tag::TERM_LIST: {
        TermList ts = expression->_termList;
        if (ts.isTerm()) {
          _subexpressions.push(new Expression(ts.term()));
        }
        break;
      }

#if VDEBUG
      default:
        ASSERTION_VIOLATION_REP(expression->_tag);
#endif
    }

    return expression;
  }

  bool SubformulaIterator::hasNext() {
    CALL("SubformulaIterator::hasNext");
    while (_sei.hasNext()) {
      SubexpressionIterator::Expression* expression = _sei.next();
      if (expression->isFormula()) {
        _next = expression->getFormula();
        _polarity = expression->getPolarity();
        return true;
      }
    }
    return false;
  }

  bool SubtermIterator::hasNext() {
    CALL("SubtermIterator::hasNext");
    while (_sei.hasNext()) {
      SubexpressionIterator::Expression* expression = _sei.next();
      if (expression->isTermList()) {
        _next = expression->getTermList();
        return true;
      }
    }
    return false;
  }
}