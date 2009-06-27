/**
 * @file Clause.cpp
 * Implements class BDD for binary decision diagrams
 */

#include "../Lib/Int.hpp"
#include "../Lib/Stack.hpp"
#include "../Lib/DHMap.hpp"

#include "../Lib/Timer.hpp"
#include "../Lib/Environment.hpp"

#include "../Lib/Exception.hpp"

#include "BDD.hpp"

int gBDDTime=0;

namespace Kernel {

using namespace Lib;

BDD* BDD::instance()
{
  CALL("BDD::instance");

  static BDD* inst=0;
  if(!inst) {
    inst=new BDD();
  }
  return inst;
}

BDD::BDD()
: _newVar(0)
{
  _trueNode._var=-1;
  _falseNode._var=-1;
#ifdef VDEBUG
  _trueNode._pos=0;
  _trueNode._neg=0;
  _falseNode._pos=0;
  _falseNode._neg=0;
#endif
}

BDD::~BDD()
{
  CALL("BDD::~BDD");

  NodeSet::Iterator nit(_nodes);
  while(nit.hasNext()) {
    BDDNode* node=nit.next();
    delete node;
  }
}

BDDNode* BDD::getAtomic(int varNum, bool positive)
{
  CALL("BDD::getAtomic");
  ASS_GE(varNum,0);

  if(varNum>=_newVar) {
    _newVar=varNum+1;
  }

  if(positive) {
    return getNode(varNum, getTrue(), getFalse());
  } else {
    return getNode(varNum, getFalse(), getTrue());
  }
}

BDDNode* BDD::conjunction(BDDNode* n1, BDDNode* n2)
{
  CALL("BDD::conjunction");
  return getBinaryFnResult(n1,n2, ConjunctionFn(this));
}

BDDNode* BDD::disjunction(BDDNode* n1, BDDNode* n2)
{
  CALL("BDD::disjunction");
  return getBinaryFnResult(n1,n2, DisjunctionFn(this));
}

BDDNode* BDD::xOrNonY(BDDNode* x, BDDNode* y)
{
  CALL("BDD::xOrNonY");
  return getBinaryFnResult(x,y, XOrNonYFn(this));
}


bool BDD::isXOrNonYConstant(BDDNode* x, BDDNode* y, bool resValue)
{
  CALL("BDD::isXOrNonYConstant");
  return hasConstantResult(x,y, resValue, XOrNonYFn(this));
}

/**
 * Return pointer to BDDNode that represents result of applying
 * the binary function specified by the BinBoolFn functor to
 * @b n1 and @n2. The binary functor BinBoolFn must allow to be
 * called as
 *
 * BDDNode* BinBoolFn(BDDNode* m1, BDDNode* m2)
 *
 * and return either result of applying the represented binary
 * function to @b m1 and @b m2, or 0 in case the result cannot
 * be determined by merely examining the objects pointed by
 * @b m1 and @b m2. It mustn't return 0 if both arguments are
 * either true or false BDD.
 */
template<class BinBoolFn>
BDDNode* BDD::getBinaryFnResult(BDDNode* n1, BDDNode* n2, BinBoolFn fn)
{
  CALL("BDD::getBinaryFnResult");
  ASS(n1);
  ASS(n2);

  int counter=0;
  int initTime=0;

  static Stack<BDDNode*> toDo(8);
  //results stack contains zeroes and proper pointers standing for
  //intermediate results.
  //It can be viewed as a prefix of an expression in prefix notation
  //with 0 being a binary function and non-zeroes constants.
  //The expression is being simplified every time a well formed
  //subexpression (i.e. zero followed by two non-zeroes) appears.
  static Stack<BDDNode*> results(8);
  //Variable numbers to be used for building intermediate results in
  //the results stack.
  static Stack<int> vars(8);

  static DHMap<pair<BDDNode*,BDDNode*>, BDDNode*, PtrPairSimpleHash > cache;
  //if the cache was not reset, too much memory would be consumed
  cache.reset();

  for(;;) {
    counter++;
    if(counter==500) {
      if(initTime==0) {
	initTime=env.timer->elapsedMilliseconds();
      }
    }
    if(counter==50000) {
      counter=0;
      if(env.timeLimitReached()) {
	throw TimeLimitExceededException();
      }
    }
    BDDNode* res=fn(n1,n2);
    if(res || cache.find(make_pair(n1, n2), res)) {
      while(results.isNonEmpty() && results.top()!=0) {
	BDDNode* pos=results.pop();
	BDDNode* neg=res;
	unsigned var=vars.pop();
	if(pos==neg) {
	  res=pos;
	} else {
	  res=getNode(var, pos, neg);
	}
	ASS_EQ(results.top(),0);
	results.pop();
	BDDNode* arg1=results.pop();
	BDDNode* arg2=results.pop();
	if( !(counter%4) ) {
	  cache.insert(make_pair(arg1, arg2), res);
	}
      }
      results.push(res);
    } else {
      //we split at variables with higher numbers first
      int splitVar=max(n1->_var, n2->_var);
      ASS_GE(splitVar,0);
      toDo.push((n2->_var==splitVar) ? n2->_neg : n2);
      toDo.push((n1->_var==splitVar) ? n1->_neg : n1);
      toDo.push((n2->_var==splitVar) ? n2->_pos : n2);
      toDo.push((n1->_var==splitVar) ? n1->_pos : n1);
      results.push(n2);
      results.push(n1);
      results.push(0);
      //now push arguments at the stack, so that we know store the
      //answer into the cache
      vars.push(splitVar);
    }

    if(toDo.isEmpty()) {
      break;
    }
    n1=toDo.pop();
    n2=toDo.pop();
  }

  if(initTime) {
    gBDDTime+=env.timer->elapsedMilliseconds()-initTime;
  }

  ASS(toDo.isEmpty());
  ASS_EQ(results.length(),1);
  return results.pop();
}

/**
 * Return true iff the result of applying the binary function specified
 * by the BinBoolFn functor to @b n1 and @n2 is a constant BDD with truth
 * value equal to @b resValue.
 *
 * The binary functor BinBoolFn must allow to be called as
 *
 * BDDNode* BinBoolFn(BDDNode* m1, BDDNode* m2)
 *
 * and return either result of applying the represented binary
 * function to @b m1 and @b m2, or 0 in case the result cannot
 * be determined by merely examining the objects pointed by
 * @b m1 and @b m2. It mustn't return 0 if both arguments are
 * either true or false BDD.
 */
template<class BinBoolFn>
bool BDD::hasConstantResult(BDDNode* n1, BDDNode* n2, bool resValue, BinBoolFn fn)
{
  CALL("BDD::hasConstantResult");
  ASS(n1);
  ASS(n2);

  int counter=0;
  int initTime=0;

  static Stack<BDDNode*> toDo(8);
  toDo.reset();

  static DHMap<pair<BDDNode*,BDDNode*>, bool, PtrPairSimpleHash > cache;
  cache.reset();

  for(;;) {
    counter++;
    if(counter==500) {
      if(initTime==0) {
	initTime=env.timer->elapsedMilliseconds();
      }
    }
    if(counter==50000) {
      counter=0;
      if(env.timeLimitReached()) {
	throw TimeLimitExceededException();
      }
    }
    BDDNode* res=fn(n1,n2);
    if(res) {
      if( (resValue && !isTrue(res)) ||
	      (!resValue && !isFalse(res))) {
	if(initTime) {
	  gBDDTime+=env.timer->elapsedMilliseconds()-initTime;
	}
	return false;
      }
    } else {
      if(!cache.find(make_pair(n1, n2)))
      {
	//we split at variables with higher numbers first
	int splitVar=max(n1->_var, n2->_var);
	ASS_GE(splitVar,0);
	toDo.push((n2->_var==splitVar) ? n2->_neg : n2);
	toDo.push((n1->_var==splitVar) ? n1->_neg : n1);
	toDo.push((n2->_var==splitVar) ? n2->_pos : n2);
	toDo.push((n1->_var==splitVar) ? n1->_pos : n1);

	if( !(counter%4) ) {
	  cache.insert(make_pair(n1, n2), false);
	}
      }
    }

    if(toDo.isEmpty()) {
      break;
    }
    n1=toDo.pop();
    n2=toDo.pop();
  }

  if(initTime) {
    gBDDTime+=env.timer->elapsedMilliseconds()-initTime;
  }

  return true;
}


BDDNode* BDD::ConjunctionFn::operator()(BDDNode* n1, BDDNode* n2)
{
  if(_parent->isFalse(n1) || _parent->isFalse(n2)) {
    return _parent->getFalse();
  }
  if(_parent->isTrue(n1)) {
    return n2;
  }
  if(_parent->isTrue(n2)) {
    return n1;
  }
  return 0;
}

BDDNode* BDD::DisjunctionFn::operator()(BDDNode* n1, BDDNode* n2)
{
  if(_parent->isTrue(n1) || _parent->isTrue(n2)) {
    return _parent->getTrue();
  }
  if(_parent->isFalse(n1)) {
    return n2;
  }
  if(_parent->isFalse(n2)) {
    return n1;
  }
  return 0;
}

BDDNode* BDD::XOrNonYFn::operator()(BDDNode* n1, BDDNode* n2)
{
  if(_parent->isTrue(n1) || _parent->isFalse(n2)) {
    return _parent->getTrue();
  }
  if(_parent->isTrue(n2)) {
    return n1;
  }
  return 0;
}


BDDNode* BDD::getNode(int varNum, BDDNode* pos, BDDNode* neg)
{
  CALL("BDD::getNode");
  ASS_GE(varNum,0);
  ASS_L(varNum,_newVar);
  ASS_NEQ(pos,neg);

  //newNode contains either 0 or pointer to a BDDNode that
  //hasn't been used yet.
  static BDDNode* newNode=0;

  if(newNode==0) {
    newNode=new BDDNode();
  }

  newNode->_var=varNum;
  newNode->_pos=pos;
  newNode->_neg=neg;

  BDDNode* res=_nodes.insert(newNode);
  if(res==newNode) {
    newNode=0;
  }
  return res;
}


string BDD::toString(BDDNode* node)
{
  string res="";
  static Stack<BDDNode*> nodes(8);
  nodes.push(node);
  while(nodes.isNonEmpty()) {
    BDDNode* n=nodes.pop();
    if(n==0) {
      res+=") ";
    } else if(isTrue(n)) {
      res+="$true ";
    } else if(isFalse(n)) {
      res+="$false ";
    } else {
      res+=string("( ")+Int::toString(n->_var)+" ? ";
      nodes.push(0);
      nodes.push(n->_neg);
      nodes.push(n->_pos);
    }
  }
  return res;
}

string BDD::toTPTPString(BDDNode* node)
{
  if(isTrue(node)) {
    return "$true";
  } else if(isFalse(node)) {
    return "$false";
  } else {
    return string("( ( bddPred")+Int::toString(node->_var)+" => "+toTPTPString(node->_pos)+
      ") & ( ~bddPred"+Int::toString(node->_var)+" => "+toTPTPString(node->_neg)+" ) )";
  }
}


bool BDD::equals(const BDDNode* n1,const BDDNode* n2)
{
  return n1->_var==n2->_var && n1->_pos==n2->_pos && n1->_neg==n2->_neg;
}
unsigned BDD::hash(const BDDNode* n)
{
  CALL("BDD::hash");

  unsigned res=Hash::hash(n->_var);
  res=Hash::hash(n->_pos, res);
  res=Hash::hash(n->_neg, res);
  return res;
}


void BDDConjunction::addNode(BDDNode* n)
{
  CALL("BDDConjunction::addNode");

  if(_isFalse) {
    return;
  }
  if(_bdd->isConstant(n)) {
    if(_bdd->isFalse(n)) {
      _isFalse=true;
    } else {
      ASS(_bdd->isTrue(n));
    }
    return;
  }

  if(n->_var > _maxVar) {
    _maxVar=n->_var;
  }

//  int nodeCnt=_nodes.size();
//  int satNodes=nodeCnt;

  NodeList* next=_nodes;
  NodeList::push(n, _nodes);
  NodeList* prev=_nodes;

//  _nodes.push(n);
//  nodeCnt++;

  bool assignmentChanged;
  if(!findNextSatAssignment(n, assignmentChanged)) {
    _isFalse=true;
    return;
  }
  if(!assignmentChanged) {
    return;
  }

//  int counter=0;

  while(next) {
    if(findNextSatAssignment(next->head(), assignmentChanged)) {
      if(assignmentChanged) {
	//We put the current node (next) at the beginning of the _nodes list and
	//continue with its second element.
	prev->setTail(next->tail());
	prev=next;
	prev->setTail(_nodes);
	next=_nodes;
	_nodes=prev;

	_decisionPnts.makeEmpty();
      } else {
	prev=next;
	next=next->tail();
      }
    } else {
      _isFalse=true;
      return;
    }
//    counter++;
//    if(counter==50000) {
//      counter=0;
//      cout<<"---- "<<_bdd->toString(n)<<"\n";
//      printAssignment();
//    }
  }

//  cout<<"Added "<<_bdd->toString(n)<<"\n";
//  printAssignment();
}

void BDDConjunction::printAssignment()
{
  for(int i=_assignment.size()-1;i>=0;i--) {
    cout<<_assignment[i];
    if(i%10==0) {
      cout<<'\t'<<i<<endl;
    }
//    cout<<i<<'\t'<<_assignment[i]<<endl;
  }
}


bool BDDConjunction::findNextSatAssignment(BDDNode* n0, bool& assignmentChanged)
{
  ASS(!_bdd->isConstant(n0));
  assignmentChanged=false;


  static Stack<BDDNode*> decPnts;
  decPnts.reset();
#if VDEBUG
  bool alreadyRestarted=false;
#endif

  BDDNode* n=n0;

satSeekStart:
  while(!_bdd->isConstant(n)) {
    if(_assignment[n->_var]) {
      n=n->_pos;
    } else {
//      if(decPnts.isNonEmpty()) {
//	ASS_G(decPnts.top()->_var, n->_var);
//      }
      decPnts.push(n);
//      cout<<"dp_push"<<n->_var<<"\n";
      n=n->_neg;
    }
  }
  if(_bdd->isTrue(n)) {

    n=n0;
    while(!_bdd->isConstant(n)) {
      if(_assignment[n->_var]) {
        n=n->_pos;
      } else {
        _decisionPnts.insert(n->_var);
        n=n->_neg;
      }
    }
    ASS(_bdd->isTrue(n));
    return true;
  }
  assignmentChanged=true;

  int changed;

  if(decPnts.isEmpty()) {
#if VDEBUG
    //the BDD n0 is non-constant, so it must be satisfiable
    ASS(!alreadyRestarted);
    alreadyRestarted=true;
#endif
//    changed=n0->_var;
//    while(++changed<=_maxVar && _assignment[changed]) {}
//    if(changed>_maxVar) {
//      return false;
//    }
//    while(++changed<=_maxVar && _assignment[changed]) {}
    if(!_decisionPnts.findLeastGreater(n0->_var, changed)) {
      return false;
    }

    decPnts.reset();
//    cout<<"dp_reset\n";
    n=n0;
  } else {
    BDDNode* decPnt=decPnts.pop();
    changed=decPnt->_var;
//    cout<<"dp_pop"<<changed<<"\n";

    n=decPnt->_pos;
  }
  ASS(!_assignment[changed]);
  _assignment[changed]=true;
  for(int i=0;i<changed;i++) {
    _assignment[i]=false;
  }
  goto satSeekStart;

}



}
