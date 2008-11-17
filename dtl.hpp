/*
  dtl-0.05 -- Diff Template Library
  Copyright(C) 2008-  Tatsuhiko Kubo <cubicdaiya@gmail.com>
*/

#ifndef DTL_H
#define DTL_H

#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <iostream>

namespace dtl {

  /**
   * type of edit for SES
   */
  typedef int editType;
  const   editType SES_DELETE = -1;
  const   editType SES_COMMON = 0;
  const   editType SES_ADD    = 1;

  /**
   * mark of SES
   */
  const std::string SES_MARK_DELETE = "-";
  const std::string SES_MARK_COMMON = " ";
  const std::string SES_MARK_ADD    = "+";

  /**
   * info for Unified Format
   */
  typedef struct eleminfo {
    int      beforeIdx;
    int      afterIdx;
    editType type;
  } elemInfo;

  #define SEPARATE_SIZE (3)
  #define CONTEXT_SIZE (3)

  /**
   * cordinate for registering route
   */
  typedef struct Point {
    int x;
    int y;
    int k;
  } P;

  const unsigned int MAX_CORDINATES_SIZE = 2000000;
  
  template <typename elem>
  class Sequence
  {
  public :
    typedef std::vector<elem> _Sequence;
    Sequence () {}
    virtual ~Sequence () {}

    _Sequence getSequence () {
      return sequence;
    }
    void addSequence (elem e) {
      sequence.push_back(e);
    }
  protected :
    _Sequence sequence;
  };

  template <typename elem>
  class Lcs : public Sequence<elem>
  {
  public :
    Lcs ()  {}
    ~Lcs () {}
  };

  template <typename elem>
  class Ses : public Sequence<elem>
  {
  private :
    typedef std::pair<elem, elemInfo> sesElem;
    typedef std::vector< sesElem > sesSequence;
  public :
    
    Ses () {
      onlyAdd    = true;
      onlyDelete = true;
      onlyCopy   = true;
    }
    
    ~Ses () {}
    
    bool isOnlyAdd () {
      return onlyAdd;
    }
    
    bool isOnlyDelete () {
      return onlyDelete;
    }
    
    bool isOnlyCopy () {
      return onlyCopy;
    }
    
    bool isOnlyOneOperation () {
      return isOnlyAdd() || isOnlyAdd() || isOnlyCopy();
    }
    
    bool isChange () {
      return !onlyCopy;
    }
    
    using Sequence<elem>::addSequence;
    void addSequence (elem e, int beforeIdx, int afterIdx, editType type) {
      elemInfo info;
      info.beforeIdx = beforeIdx;
      info.afterIdx  = afterIdx;
      info.type      = type;
      sesElem pe(e, info);
      sequence.push_back(pe);
      switch (type) {
      case SES_DELETE:
	onlyCopy   = false;
	onlyAdd    = false;
	break;
      case SES_COMMON:
	onlyAdd    = false;
	onlyDelete = false;
	break;
      case SES_ADD:
	onlyDelete = false;
	onlyCopy   = false;
	break;
      }
    }
    
    sesSequence getSequence () {
      return sequence;
    }
  private :
    sesSequence sequence;
    bool onlyAdd;
    bool onlyDelete;
    bool onlyCopy;
  };

  typedef std::vector<int> editPath;
  typedef std::vector<P>   editPathCordinates;

  template <typename elem, typename sequence>
  class Diff
  {
  private :
    sequence A;
    sequence B;
    int M;
    int N;
    int delta;
    int offset;
    int *fp;
    int editDistance;
    Lcs<elem> lcs;
    Ses<elem> ses;
    editPath path;
    editPathCordinates pathCordinates;
    bool reverse;

    /**
     * Unified Format Hunk
     */
    typedef std::pair<elem, elemInfo> sesElem;
    typedef struct unihunk {
      int a, b, c, d;                 // @@ -a,b +c,d @@
      std::vector<sesElem> common[2]; // anteroposterior commons on changes
      std::vector<sesElem> change;    // changes
    } uniHunk;
    std::vector<uniHunk> uniHunks;

  public :
    Diff(sequence& A, sequence& B) {
      M = std::distance(A.begin(), A.end());
      N = std::distance(B.begin(), B.end());
      if (M < N) {
	this->A = A;
	this->B = B;
	reverse = false;
      } else {
	this->A = B;
	this->B = A;
	std::swap(M, N);
	reverse = true;
      }
      editDistance = 0;
      delta = N - M;
      offset = M + 1;
      int size = M + N + 3;
      fp = new int[size];
      std::fill(&fp[0], &fp[size], -1);
      path = editPath(size);
      std::fill(path.begin(), path.end(), -1);
    }

    ~Diff() {
      delete[] this->fp;
    }

    int getEditDistance() {
      return editDistance;
    }

    Lcs<elem> getLcs() {
      return lcs;
    }

    Ses<elem> getSes() {
      return ses;
    }

    bool isReverse () {
      return reverse;
    }

    sequence patch (sequence seq, Ses<elem>& ses) {
      std::vector< std::pair<elem, elemInfo> > sesSeq = ses.getSequence();
      std::list<elem> seqLst(seq.begin(), seq.end());
      std::list< std::pair<elem, elemInfo> > sesLst(sesSeq.begin(), sesSeq.end());
      typename std::list<elem>::iterator lstIt = seqLst.begin();
      typename std::vector< std::pair<elem, elemInfo> >::iterator sesIt;
      for (sesIt=sesSeq.begin();sesIt!=sesSeq.end();++sesIt) {
	switch (sesIt->second.type) {
	case SES_ADD :
	  seqLst.insert(lstIt, sesIt->first);
	  break;
	case SES_DELETE :
	  lstIt = seqLst.erase(lstIt);
	  break;
	case SES_COMMON :
	  ++lstIt;
	  break;
	default :
	  break;
	}
      }
      sequence patchedSeq(seqLst.begin(), seqLst.end());
      return patchedSeq;
    }
    
    void compose(bool huge = false) {

      if (huge) {
	pathCordinates.reserve(MAX_CORDINATES_SIZE + 50000);
      }

      // O(NP) Algorithm
      int p = -1;
      int k;
    ONP:
      do {
	++p;
	for (k=-p;k<=delta-1;++k) {
	  fp[k+offset] = snake(k, fp[k-1+offset]+1, fp[k+1+offset]);
	}
	for (k=delta+p;k>=delta+1;--k) {
	  fp[k+offset] = snake(k, fp[k-1+offset]+1, fp[k+1+offset]);
	}
	fp[delta+offset] = snake(delta, fp[delta-1+offset]+1, fp[delta+1+offset]);
      } while (fp[delta+offset] != N && pathCordinates.size() < MAX_CORDINATES_SIZE);

      editDistance += delta + 2 * p;
      int r = path[delta+offset];
      P cordinate;
      editPathCordinates epc(0);

      while(r != -1){
	cordinate.x = pathCordinates[r].x;
	cordinate.y = pathCordinates[r].y;
	epc.push_back(cordinate);
	r = pathCordinates[r].k;
      }

      // recordSequence Longest Common Subsequence & Shortest Edit Script
      if (!recordSequence(epc)) {
	pathCordinates.resize(0);
	epc.resize(0);
	p = -1;
	goto ONP;
      }
    }

    void printUnifiedFormat () {
      typename std::vector<uniHunk>::iterator uit;
      for (uit=uniHunks.begin();uit!=uniHunks.end();++uit) {
	// header
	std::cout << "@@" 
		  << " -" << uit->a << "," << uit->b 
		  << " +" << uit->c << "," << uit->d 
		  << " @@" << std::endl;

	// header commons
	typename std::vector<sesElem>::iterator vit;
	for (vit=uit->common[0].begin();vit!=uit->common[0].end();++vit) {
	  std::cout << SES_MARK_COMMON << vit->first << std::endl;
	}
	// changes
	for (vit=uit->change.begin();vit!=uit->change.end();++vit) {
	  switch (vit->second.type) {
	  case SES_ADD:
	    std::cout << SES_MARK_ADD    << vit->first << std::endl;
	    break;
	  case SES_DELETE:
	    std::cout << SES_MARK_DELETE << vit->first << std::endl;
	    break;
	  case SES_COMMON:
	    std::cout << SES_MARK_COMMON << vit->first << std::endl;
	    break;
	  }
	}
	// footer commons
	for (vit=uit->common[1].begin();vit!=uit->common[1].end();++vit) {
	  std::cout << " " << vit->first << std::endl;
	}
      }
    }
    
    void composeUnifiedHunks () {
      std::vector<sesElem> common[2];
      std::vector<sesElem> change;
      std::vector<sesElem> ses_v = ses.getSequence();
      typename std::vector<sesElem>::iterator it;
      it = ses_v.begin();
      int l_cnt = 1;
      int length = std::distance(ses_v.begin(), ses_v.end());
      int middle = 0;
      bool isMiddle, isAfter;
      isMiddle = isAfter = false;
      elem e;
      elemInfo einfo;
      int a, b, c, d;         // @@ -a,b +c,d @@
      a = b = c = d = 0;
      unihunk hunk;
      std::vector<sesElem> adds;
      std::vector<sesElem> deletes;

      for (it=ses_v.begin();it!=ses_v.end();++it, ++l_cnt) {
	e = it->first;
	einfo = it->second;
	switch (einfo.type) {
	case SES_ADD :
	  middle = 0;
	  adds.push_back(*it);
	  if (!isMiddle)       isMiddle = true;
	  if (isMiddle)        ++d;
	  if (l_cnt >= length) {
	    joinSesVec(change, deletes);
	    joinSesVec(change, adds);
	    isAfter = true;
	  }
	  break;
	case SES_DELETE :
	  middle = 0;
	  deletes.push_back(*it);
	  if (!isMiddle)       isMiddle = true;
	  if (isMiddle)        ++b;
	  if (l_cnt >= length) {
	    joinSesVec(change, deletes);
	    joinSesVec(change, adds);
	    isAfter = true;
	  }
	  break;
	case SES_COMMON :
	  ++b;++d;
	  if (common[1].empty() && adds.empty() && deletes.empty() && change.empty()) {
	    if (common[0].size() < CONTEXT_SIZE) {
	      if (a == 0 && c == 0) {
		a = einfo.beforeIdx;
		c = einfo.afterIdx;
	      }
	      common[0].push_back(*it);
	    } else {
	      std::rotate(common[0].begin(), common[0].begin() + 1, common[0].end());
	      common[0].pop_back();
	      common[0].push_back(*it);
	      ++a;++c;
	      --b;--d;
	    }
	  }
	  if (isMiddle && !isAfter) {
	    ++middle;
	    typename std::vector<sesElem>::iterator vit;
	    joinSesVec(change, deletes);
	    joinSesVec(change, adds);
	    change.push_back(*it);
	    if (middle >= SEPARATE_SIZE || l_cnt >= length) {
	      isAfter = true;
	    }
	    adds.clear();
	    deletes.clear();
	  }
	  break;
	default :
	  // no through
	  break;
	}
	// print unified format
	if (isAfter && !change.empty()) {
	  typename std::vector<sesElem>::iterator cit = it;
	  int cnt = 0;
	  for (int i=0;i<SEPARATE_SIZE;++i, ++cit) {
	    if (cit->second.type == SES_COMMON) {
	      ++cnt;
	    }
	  }
	  if (cnt < SEPARATE_SIZE && l_cnt < length) {
	    middle = 0;
	    isAfter = false;
	    continue;
	  }
	  if (common[0].size() >= SEPARATE_SIZE) {
	    int c0size = common[0].size();
	    std::rotate(common[0].begin(), 
			common[0].begin() + c0size - SEPARATE_SIZE, 
			common[0].end());
	    for (int i=0;i<c0size-SEPARATE_SIZE;++i) {
	      common[0].pop_back();
	    }
	    a += c0size - SEPARATE_SIZE;
	    c += c0size - SEPARATE_SIZE;
	  }
	  if (a == 0) ++a;
	  if (c == 0) ++c;
	  if (isReverse()) std::swap(a, c);
	  hunk.a = a;hunk.b = b;hunk.c = c;hunk.d = d;
	  hunk.common[0] = common[0];
	  typename std::vector<sesElem>::iterator vit;
	  hunk.change = change;
	  hunk.common[1] = common[1];
	  uniHunks.push_back(hunk);
	  isMiddle = false;
	  isAfter = false;
	  common[0].clear();
	  common[1].clear();
	  adds.clear();
	  deletes.clear();
	  change.clear();
	  a = b = c = d = middle = 0;
	}
      }
    }
  private :
    int snake(int k, int above, int below) {
      int r;
      if (above > below) {
	r = path[k-1+offset];
      } else {
	r = path[k+1+offset];
      }

      int y = std::max(above, below);
      int x = y - k;
      while (x < M && y < N && A[x] == B[y]) {
	++x;++y;
      }

      path[k+offset] = pathCordinates.size();
      P p;
      p.x = x;p.y = y;p.k = r;
      pathCordinates.push_back(p);      
      return y;
    }

    bool recordSequence (editPathCordinates& v) {
      typename sequence::const_iterator x(A.begin());
      typename sequence::const_iterator y(B.begin());
      int x_idx, y_idx;   // line number for Unified Format
      int px_idx, py_idx; // cordinates
      int size = v.size() - 1;
      x_idx = y_idx = 1;
      px_idx = py_idx = 0;
      for (int i=size;i>=0;--i) {
	while(px_idx < v[i].x || py_idx < v[i].y) {
	  if (v[i].y - v[i].x > py_idx - px_idx) {
	    if (!isReverse()) {
	      ses.addSequence(*y, y_idx, 0, SES_ADD);
	    } else {
	      ses.addSequence(*y, y_idx, 0, SES_DELETE);
	    }
	    ++y;
	    ++y_idx;
	    ++py_idx;
	  } else if (v[i].y - v[i].x < py_idx - px_idx) {
	    if (!isReverse()) {
	      ses.addSequence(*x, x_idx, 0, SES_DELETE);
	    } else {
	      ses.addSequence(*x, x_idx, 0, SES_ADD);
	    }
	    ++x;
	    ++x_idx;
	    ++px_idx;
	  } else {             // Common
	    lcs.addSequence(*x);
	    ses.addSequence(*x, x_idx, y_idx, SES_COMMON);
	    ++x;
	    ++y;
	    ++x_idx;
	    ++y_idx;
	    ++px_idx;
	    ++py_idx;
	  }
	}
      }
      
      if (x_idx > M && y_idx > N) {
	// all recording success
      } else {
	sequence A_(A.begin() + x_idx - 1, A.end());
	sequence B_(B.begin() + y_idx - 1, B.end());
	A = A_;
	B = B_;
	M = std::distance(A.begin(), A.end());
	N = std::distance(B.begin(), B.end());
	delta = N - M;
	offset = M + 1;
	int size = M + N + 3;
	delete[] fp;
	fp = new int[size];
	std::fill(&fp[0], &fp[size], -1);
	std::fill(path.begin(), path.end(), -1);
	return false;
      }
      return true;
    }

    void recordOddSequence (int idx, int length, typename sequence::const_iterator it, const editType et) {
      while(idx < length){
	ses.addSequence(*it, idx, 0, et);
	++it;
	++idx;
	++editDistance;
      }
      ses.addSequence(*it, idx, 0, et);
      ++editDistance;
    }

    void joinSesVec (std::vector<sesElem>& s1, std::vector<sesElem>& s2) {
      typename std::vector<sesElem>::iterator vit;
      if (!s2.empty()) {
	for (vit=s2.begin();vit!=s2.end();++vit) {
	  s1.push_back(*vit);
	}
      }      
    }

  };

  /* 
   * diff3
   */
  template <typename elem, typename sequence>
  class Diff3
  {
  private:
    Diff<elem, sequence> *diff_ba;
    Diff<elem, sequence> *diff_bc;
    sequence A;
    sequence B;
    sequence C;
    int M;
    int N;
    int O;
  public :
    Diff3 () {}
    Diff3 (sequence& A, sequence& B, sequence& C) { // diff3
      this->A = A;
      this->B = B;
      this->C = C;
      this->M = std::distance(A.begin(), A.end());
      this->N = std::distance(B.begin(), B.end());
      this->O = std::distance(C.begin(), C.end());
      this->diff_ba = new Diff<elem, sequence>(B, A);
      this->diff_bc = new Diff<elem, sequence>(B, C);
    } 
    ~Diff3 () {
      delete this->diff_ba;
      delete this->diff_bc;
    }

    Diff<elem, sequence>& getDiffba () {
      return diff_ba;
    }

    Diff<elem, sequence>& getDiffbc () {
      return diff_bc;
    }

    // merge changes B to C to A
    bool merge () {
      Lcs<elem> lcs_ba = diff_ba->getLcs();
      Lcs<elem> lcs_bc = diff_bc->getLcs();
      std::vector<elem> lcs_vba = lcs_ba.getSequence();
      std::vector<elem> lcs_vbc = lcs_bc.getSequence();
      std::string lcs_sba(lcs_vba.begin(), lcs_vba.end());
      std::string lcs_sbc(lcs_vbc.begin(), lcs_vbc.end());

      Ses<elem> ses_ba = diff_ba->getSes();
      Ses<elem> ses_bc = diff_bc->getSes();
      typedef std::vector< std::pair<elem, elemInfo> > ses_v;
      ses_v sv_ba = ses_ba.getSequence();
      ses_v sv_bc = ses_bc.getSequence();
      typename ses_v::iterator it_ba;
      typename ses_v::iterator it_bc;

      std::cout << "A:" << A << std::endl;
      std::cout << "B:" << B << std::endl;
      std::cout << "C:" << C << std::endl;
      std::cout << std::endl;
      
      std::cout << "B->A" << std::endl ;
      //std::cout << lcs_sba << std::endl;
      std::cout << std::endl;
      for (it_ba=sv_ba.begin();it_ba!=sv_ba.end();++it_ba) {
	std::cout << it_ba->first 
		  << " "
		  << it_ba->second.type 
		  << std::endl;
      }
      std::cout << std::endl;
      std::cout << "B->C" << std::endl;
      //std::cout << lcs_sbc << std::endl;
      std::cout << std::endl;
      for (it_bc=sv_bc.begin();it_bc!=sv_bc.end();++it_bc) {
	std::cout << it_bc->first 
		  << " "
		  << it_bc->second.type 
		  << std::endl;
      }
      
      if (diff_ba->getEditDistance() == 0) {
	if (diff_bc->getEditDistance() == 0) {
	  // A == B == C
	  return true;
	}
	// B == A
	sequence S = diff_bc->patch(B, ses_bc);
	std::cout << ";;" << S << std::endl;
      } else {
	if (diff_bc->getEditDistance() == 0) {
	  // B == C
	  return true;
	} else {
	  // changes B to A and B to C
	  
	}
      }
      
      return true;
    }

    void compose () {
      diff_ba->compose();
      diff_bc->compose();
    }
  };
}

#endif
