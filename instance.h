#ifndef INSTANCE_H
#define INSTANCE_H 

#include <vector>
#include <list>
#include "variable.h"
#include "types.h"

typedef std::pair<int, int> Ancestral; // X --> Y is (X, Y)

class Instance {
  public:
    Instance(std::string fileName, std::string constraintsFileName);
    int getN() const;
    int getM_anc() const;
    int getM_dae() const;
    int getM_uae() const;
    int getM_aa() const;
    int getM_ord() const;
    std::string getFileName() const;
    bool isConstraint(int a, int b) const;
    bool canPruneParentLossless(int node, int j);
    bool canPruneParentHeuristic(int node, int j);
    int pruneParentSetsLossless();
    int pruneParentSetsHeuristic();
    double pruneFactor() const;

    void sortAllParents();
    std::vector< std::pair<int, int> > &getParentList();
    int numParents() const;

    Variable &getVar(int i);
    const Ancestral &getAncestral(int i) const;

    const std::vector<int> &getUndirectedExistence(int i) const;
    bool hasUndirectedForNode(int i) const;
    bool hasUndirectedExistence() const;

    friend std::ostream& operator<<(std::ostream &os, const Instance& I);

    std::vector< std::pair<int,int> > deConstraints, ueConstraints, absConstraints, ordConstraints;
  private:
    int n, dataSize, m_anc, m_dae, m_uae, m_aa, m_ord;
    std::vector< std::pair<int,int> > allParentSets;
    std::vector<Variable> vars;
    std::vector<Ancestral> ancestralConstraints;

    std::vector<std::vector<bool>> orderConstraints;

    std::vector<std::vector<int>> undirectedArcExistence;
    std::vector< std::vector<int> > mustHaveParent, mustNotHaveParent;
    std::string fileName;
};

#endif /* INSTANCE_H */
