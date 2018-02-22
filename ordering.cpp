#include"ordering.h"
#include<algorithm>
#include"debug.h"
Ordering::Ordering(int size) : size(size) {
  ordering.resize(size);
}

void Ordering::set(const int &index, const int &num) {
  ordering[index] = num;
}

int Ordering::get(const int &index) const {
  return ordering[index];
}
std::ostream& operator<<(std::ostream& os, const Ordering&o) {
  for (int i = 0; i < o.size; i++) {
    os << o.get(i) << ' ';
  }
  return os;
}

void Ordering::swap(const int &i, const int &j) {
  std::swap(ordering[i], ordering[j]);
}

Ordering Ordering::greedyOrdering(Instance &instance, int greediness) {
  int n = instance.getN();
  Ordering o(n);
  for (int i = 0; i < n; i++) {
    int v = o.findSmallestConsistentWithOrderingRandom(i, instance, greediness);
    o.set(i, v);
  }
  return o;
}

Ordering Ordering::greedyOrdering(Instance &instance) {
  int greediness = 10;
  int n = instance.getN();
  Ordering o(n);
  for (int i = 0; i < n; i++) {
    int v = o.findSmallestConsistentWithOrderingRandom(i, instance, greediness);
    o.set(i, v);
  }
  return o;
}

Ordering Ordering::randomOrdering(Instance &instance) {
  int greediness = 10;
  int n = instance.getN(), m = instance.getM();
  std::vector<int> shuffled;
  for (int i = 0; i < n; i++) {
    shuffled.push_back(i);
  }

  std::random_shuffle(shuffled.begin(), shuffled.end());
  Ordering o(n);
  for (int i = 0; i < n; i++) {
    o.set(i, shuffled[i]);
  }

  int pos[n];
  for (int i=0; i < n; i++) {
    pos[o.get(i)] = i;
  }


  // Add a while loop so that we only start with valid orderings.
  while (true) {

    bool sat = true;

    for (int i=0; i < m; i++) {
      int x = instance.getAncestral(i).first, y = instance.getAncestral(i).second;
      if (pos[x] > pos[y]) {
        o.swap(pos[x], pos[y]);
        std::swap(pos[x], pos[y]);
        sat = false;
        break;
      }
    }

    if (sat) {
      return o;
    }
  }
}

int Ordering::findSmallestConsistentWithOrdering(const int &m, Instance &instance) {
  int n = instance.getN();
  Types::Bitset current(n, 0);
  // Flag nodes that are already seen
  for (int i = 0; i < m; i++) {
    current[ordering[i]] = 1;
  }
  int minVar = -1;
  Types::Score minCost = Types::SCORE_MAX; // I assume this is sufficiently large
  for (int i = 0; i < n; i++) {
    if (!current[i]) {
      const Variable &v = instance.getVar(i);
      int numParents = v.numParents();
      for (int j = 0; j < numParents; j++) {
        const ParentSet &parentSet = v.getParent(j);
        if (parentSet.subsetOf(current) && parentSet.getScore() < minCost) {
          minVar = i;
          minCost = parentSet.getScore();
        }
      }
    }
  }
  return minVar;
}

int Ordering::findSmallestConsistentWithOrderingRandom(const int &m, Instance &instance, int MAX_HEAP_SIZE) {
  int n = instance.getN();
  std::vector<const ParentSet*> heap;

  Types::Bitset current(n, 0);
  // Flag nodes that are already seen
  for (int i = 0; i < m; i++) {
    current[ordering[i]] = 1;
  }
  for (int i = 0; i < n; i++) {
    if (!current[i]) {
      const Variable &v = instance.getVar(i);
      int numParents = v.numParents();
      for (int j = 0; j < numParents; j++) {
        const ParentSet *parentSet = &(v.getParent(j));
        if (parentSet->subsetOf(current)) {
          heap.push_back(parentSet);
          std::push_heap(heap.begin(), heap.end(), [](const ParentSet* a, const ParentSet* b) {
            return a->getScore() < b->getScore();
          });
          if (heap.size() > MAX_HEAP_SIZE) {
            std::pop_heap(heap.begin(), heap.end(), [](const ParentSet* a, const ParentSet* b) {
              return a->getScore() < b->getScore();
            });
            heap.pop_back();
          }
          break;
        }
      }
    }
  }
  const ParentSet *chosen = heap[rand()%heap.size()];
  return chosen->getVar();
}

void Ordering::insert(const int &i, const int &j) {
  if (i < j) {
    int temp = ordering[i];
    for (int k = i; k < j; k++) {
      ordering[k] = ordering[k+1];
    }
    ordering[j] = temp;
  } else {
    int temp = ordering[i];
    for (int k = i; k > j; k--) {
      ordering[k] = ordering[k-1];
    }
    ordering[j] = temp;
  }
}

void Ordering::perturb(int PERTURB_FACTOR) {
  for (int i = 0; i < PERTURB_FACTOR; i++) {
    swap(rand()%size, rand()%size);
  }
}

int Ordering::getSize() const {
  return size;
}

bool Ordering::equals(const Ordering &o) const {
  for (int i = 0; i < size; i++) {
    if (this->get(i) != o.get(i)) {
      return false;
    }
  }
  return true;
}