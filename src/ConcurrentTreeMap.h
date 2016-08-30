#pragma once
#include<atomic>
#include<limits>

#define LEFT 0
#define RIGHT 1

template<typename K, typename V>
class Node
{
	template<typename K, typename V>
	friend class ConcurrentTreeMap;
private:
	K m_key;
	V m_value;
	std::atomic<Node<K, V>*> m_child[2];

public:
	Node<K, V>();
	Node<K, V>(K key, V value);
	Node<K, V>(K key, V value, Node<K, V>* left, Node<K, V>* right);
};

template <typename K, typename V>
Node<K, V>::Node()
{

}

template <typename K, typename V>
Node<K, V>::Node(K key, V value)
{
	m_key = key;
	m_value = value;
}

template <typename K, typename V>
Node<K, V>::Node(K key, V value, Node<K, V>* left, Node<K, V>* right)
{
	m_key = key;
	m_value = value;
	m_child[0] = left;
	m_child[1] = right;
}

template<typename K, typename V>
class ConcurrentTreeMap
{
private:
	Node<K, V>* m_root;
private:
	inline bool CAS(Node<K, V>* parent, int which, Node<K, V>* oldChild, Node<K, V>* newChild);
	inline void seek(K key, Node<K, V>** parent, Node<K, V>** node, Node<K, V>** injectionPoint);
	Node<K, V>* getAddress(Node<K, V>* p);
	inline bool isNull(Node<K, V>* p);
	inline bool isLocked(Node<K, V>* p);
	inline Node<K, V>* setLock(Node<K, V>* p);
	inline Node<K, V>* unsetLock(Node<K, V>* p);
	inline Node<K, V>* setNull(Node<K, V>* p);
	inline Node<K, V>* newLeafNode(K key, V value);
	inline bool lockEdge(Node<K, V>* parent, Node<K, V>* oldChild, int which, bool n);
	inline void unlockEdge(Node<K, V>* parent, int which);
	inline bool findSmallest(Node<K, V>* node, Node<K, V>* rChild, Node<K, V>** succNode, Node<K, V>** succParent);
	inline unsigned long getSize(Node<K, V>* node, unsigned long sizeSoFar);
	inline bool isValidBST(Node<K, V>* node, K min, K max);

public:
	ConcurrentTreeMap();

	V lookup(const K key);
	bool insert(K key, V value);
	bool remove(K key);
	unsigned long size();
	bool isValidTree();
};

template<typename K, typename V>
ConcurrentTreeMap<K, V>::ConcurrentTreeMap()
{
	m_root = newLeafNode(std::numeric_limits<K>::min(), std::numeric_limits<V>::min());
	m_root->m_child[RIGHT] = newLeafNode(std::numeric_limits<K>::max(), std::numeric_limits<V>::max());
}

template<typename K, typename V>
Node<K, V>* ConcurrentTreeMap<K, V>::getAddress(Node<K, V>* p)
{
	return (Node<K, V>*)((uintptr_t)p & ~((uintptr_t)3));
}
template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::isNull(Node<K, V>* p)
{
	return ((uintptr_t)p & 2) != 0;
}

template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::isLocked(Node<K, V>* p)
{
	return ((uintptr_t)p & 1) != 0;
}

template<typename K, typename V>
inline Node<K, V>* ConcurrentTreeMap<K, V>::setLock(Node<K, V>* p)
{
	return (Node<K, V>*) ((uintptr_t)p | 1);
}

template<typename K, typename V>
inline Node<K, V>* ConcurrentTreeMap<K, V>::unsetLock(Node<K, V>* p)
{
	return (Node<K, V>*) ((uintptr_t)p & ~((uintptr_t)1));
}

template<typename K, typename V>
inline Node<K, V>* ConcurrentTreeMap<K, V>::setNull(Node<K, V>* p)
{
	return (Node<K, V>*) ((uintptr_t)p | 2);
}

template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::CAS(Node<K, V>* parent, int which, Node<K, V>* oldChild, Node<K, V>* newChild)
{
	if (parent->m_child[which] == oldChild)
	{
		return parent->m_child[which].compare_exchange_strong(oldChild, newChild, std::memory_order_seq_cst);
	}
	else
	{
		return false;
	}
}

template<typename K, typename V>
inline Node<K, V>* ConcurrentTreeMap<K, V>::newLeafNode(K key, V value)
{
	return new Node<K, V>(key, value, setNull(nullptr), setNull(nullptr));
}

template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::lockEdge(Node<K, V>* parent, Node<K, V>* oldChild, int which, bool n)
{
	if (isLocked(parent->m_child[which]))
	{
		return false;
	}
	Node<K, V>* newChild;
	newChild = setLock(oldChild);
	if (n)
	{
		if (CAS(parent, which, setNull(oldChild), setNull(newChild)))
		{
			return true;
		}
	}
	else
	{
		if (CAS(parent, which, oldChild, newChild))
		{
			return true;
		}
	}
	return false;
}

template<typename K, typename V>
inline void ConcurrentTreeMap<K, V>::unlockEdge(Node<K, V>* parent, int which)
{
	parent->m_child[which] = unsetLock(parent->m_child[which]);
}

template<typename K, typename V>
inline void ConcurrentTreeMap<K, V>::seek(K key, Node<K, V>** parent, Node<K, V>** node, Node<K, V>** injectionPoint)
{
	Node<K, V>* prev[2];
	Node<K, V>* curr[2];
	Node<K, V>* lastRNode[2];
	Node<K, V>* address[2];
	K lastRKey[2];
	K cKey;
	Node<K, V>* temp;
	int which;
	int pSeek;
	int cSeek;
	int index;

	pSeek = 0; cSeek = 1;
	prev[pSeek] = nullptr;	curr[pSeek] = nullptr;
	lastRNode[pSeek] = nullptr;	lastRKey[pSeek] = 0;
	address[pSeek] = nullptr;

BEGIN:
	{
		//initialize all variables use in traversal
		prev[cSeek] = m_root; curr[cSeek] = m_root->m_child[RIGHT];
		lastRNode[cSeek] = m_root; lastRKey[cSeek] = std::numeric_limits<K>::min();
		address[cSeek] = nullptr;

		while (true)
		{
			//read the key stored in the current node
			cKey = curr[cSeek]->m_key;
			if (key<cKey)
			{
				which = LEFT;
			}
			else if (key > cKey)
			{
				which = RIGHT;
			}
			else
			{
				//key found; stop the traversal
				index = cSeek;
				goto END;
			}
			temp = curr[cSeek]->m_child[which];
			address[cSeek] = getAddress(temp);
			if (isNull(temp))
			{
				//null flag set; reached a leaf node
				if (lastRNode[cSeek]->m_key != lastRKey[cSeek])
				{
					goto BEGIN;
				}
				if (!isLocked(lastRNode[cSeek]->m_child[RIGHT]))
				{
					//key stored in the node at which the last right edge was traversed has not changed
					index = cSeek;
					goto END;
				}
				else if (lastRNode[pSeek] == lastRNode[cSeek] && lastRKey[pSeek] == lastRKey[cSeek])
				{
					index = pSeek;
					goto END;
				}
				else
				{
					pSeek = 1 - pSeek;
					cSeek = 1 - cSeek;
					goto BEGIN;
				}
			}
			if (which == RIGHT)
			{
				//the next edge that will be traversed is the right edge; keep track of the current node and its key
				lastRNode[cSeek] = curr[cSeek];
				lastRKey[cSeek] = cKey;
			}
			//traverse the next edge
			prev[cSeek] = curr[cSeek];	curr[cSeek] = address[cSeek];
			//determine if the most recent edge traversed is marked
		}
	}
END:
	{
		//initialize the seek record and return
		*parent = prev[index];
		*node = curr[index];
		*injectionPoint = address[index];
		return;
	}
}

template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::findSmallest(Node<K, V>* node, Node<K, V>* rChild, Node<K, V>** succNode, Node<K, V>** succParent)
{
	//find the node with the smallest key in the subtree rooted at the right child
	Node<K, V>* prev;
	Node<K, V>* curr;
	Node<K, V>* lChild;
	Node<K, V>* temp;
	bool n;

	prev=node; curr=rChild;
	while(true)
	{
		temp = curr->m_child[LEFT];
		n = isNull(temp); lChild = getAddress(temp);
		if(n)
		{
			break;
		}
		//traverse the next edge
		prev = curr; curr = lChild;
	}
	//initialize successor key seek record and return
	*succNode = curr;
	*succParent = prev;
	if(prev == node)
	{
		return true;
	}
	else
	{
		return false;
	}
}

template<typename K, typename V>
V ConcurrentTreeMap<K, V>::lookup(const K key)
{
	Node<K, V>* parent;
	Node<K, V>* node;
	Node<K, V>* injectionPoint;

	seek(key, &parent, &node, &injectionPoint);
	if (node->m_key == key)
	{
		return node->m_value;
	}
	else
	{
		return std::numeric_limits<V>::max();
	}
}

template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::insert(K key, V value)
{
	Node<K, V>* parent;
	Node<K, V>* node;
	Node<K, V>* injectionPoint;
	K nKey;
	int which;
	bool result;
	while (true)
	{
		seek(key, &parent, &node, &injectionPoint);
		nKey = node->m_key;
		if (nKey == key)
		{
			return false;
		}
		//create a new node and initialize its fields
		Node<K, V>* newNode = newLeafNode(key, value);
		which = key < nKey ? LEFT : RIGHT;
		result = CAS(node, which, setNull(injectionPoint), newNode);
		if (result)
		{
			return true;
		}
	}
}

template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::remove(K key)
{
	Node<K, V>* parent;
	Node<K, V>* node;
	Node<K, V>* lChild;
	Node<K, V>* rChild;
	Node<K, V>* temp;
	Node<K, V>* injectionPoint;
	K nKey;
	K pKey;
	int pWhich;
	bool ln;
	bool rn;
	bool lLock;
	bool rLock;

	while(true)
	{
		seek(key, &parent, &node, &injectionPoint);
		nKey = node->m_key;
		if(nKey != key)
		{
			return false;
		}
		pKey = parent->m_key;
		temp = node->m_child[LEFT];
		lChild = getAddress(temp); ln = isNull(temp); lLock = isLocked(temp);
		temp = node->m_child[RIGHT];
		rChild = getAddress(temp); rn = isNull(temp); rLock = isLocked(temp);
		pWhich = nKey < pKey ? LEFT: RIGHT;
		if(ln || rn) //simple delete
		{
			if(lockEdge(parent, node, pWhich, false))
			{
				if(lockEdge(node, lChild, LEFT, ln))
				{
					if(lockEdge(node, rChild, RIGHT, rn))
					{
						if(nKey != node->m_key)
						{
							unlockEdge(parent, pWhich);
							unlockEdge(node, LEFT);
							unlockEdge(node, RIGHT);
							continue;
						}
						if(ln && rn) //00 case
						{
							parent->m_child[pWhich] = setNull(node);
						}
						else if(ln) //01 case
						{
							parent->m_child[pWhich] = rChild;
						}
						else //10 case
						{
							parent->m_child[pWhich] = lChild;
						}
						return true;
					}
					else
					{
						unlockEdge(parent, pWhich);
						unlockEdge(node, LEFT);
					}
				}
				else
				{
					unlockEdge(parent, pWhich);
				}
			}
		}
		else //complex delete
		{
			Node<K, V>* succNode;
			Node<K, V>* succParent;
			Node<K, V>* succNodeLChild;
			Node<K, V>* succNodeRChild;
			Node<K, V>* temp;
			bool isSplCase;
			bool srn;

			isSplCase = findSmallest(node, rChild, &succNode, &succParent);

			succNodeLChild = getAddress(succNode->m_child[LEFT]);
			temp = succNode->m_child[RIGHT];
			srn = isNull(temp); succNodeRChild = getAddress(temp);

			if(!lockEdge(node, rChild, RIGHT, false))
			{
				continue;
			}
			while(true)
			{
				if(!isSplCase) //common case
				{
					if(lockEdge(succParent, succNode, LEFT, false))
					{
						if(lockEdge(succNode, succNodeLChild, LEFT, true))
						{
							if(lockEdge(succNode, succNodeRChild, RIGHT, srn))
							{
								if(nKey == node->m_key)
								{
									node->m_key = succNode->m_key;
									if(srn)
									{
										succParent->m_child[LEFT] = setNull(succNode);
									}
									else
									{
										succParent->m_child[LEFT] = succNodeRChild;
									}
									unlockEdge(node, RIGHT);
									return true;
								}
								else
								{
									unlockEdge(node, RIGHT);
									unlockEdge(succParent,LEFT);
									unlockEdge(succNode, LEFT);
									unlockEdge(succNode, RIGHT);
									break;
								}
							}
							else
							{
								unlockEdge(succParent,LEFT);
								unlockEdge(succNode, LEFT);
							}
						}
						else
						{
							unlockEdge(succParent,LEFT);
						}
					}
				}
				else //spl case
				{
					if(lockEdge(succNode, succNodeLChild, LEFT, true))
					{
						if(lockEdge(succNode, succNodeRChild, RIGHT, srn))
						{
							if(nKey == node->m_key)
							{
								node->m_key = succNode->m_key;
								if(srn)
								{
									succParent->m_child[RIGHT] = setNull(succNode);
								}
								else
								{
									succParent->m_child[RIGHT] = succNodeRChild;
								}
								return true;
							}
							else
							{
								unlockEdge(node, RIGHT);
								unlockEdge(succNode, LEFT);
								unlockEdge(succNode, RIGHT);
								break;
							}
						}
						else
						{
							unlockEdge(succNode, LEFT);
						}
					}
				}
				isSplCase = findSmallest(node, rChild, &succNode, &succParent);

				succNodeLChild = getAddress(succNode->m_child[LEFT]);
				temp = succNode->m_child[RIGHT];
				srn = isNull(temp); succNodeRChild = getAddress(temp);
			}
		}
	}
}

template<typename K, typename V>
inline unsigned long ConcurrentTreeMap<K, V>::getSize(Node<K, V>* node, unsigned long sizeSoFar)
{
	if(isNull(node))
	{
		return sizeSoFar;
	}
	return 1 + getSize(node->m_child[LEFT], sizeSoFar) + getSize(node->m_child[RIGHT], sizeSoFar);
}

template<typename K, typename V>
unsigned long ConcurrentTreeMap<K, V>::size()
{
	return getSize(m_root, 0) - 2;
}

template<typename K, typename V>
inline bool ConcurrentTreeMap<K, V>::isValidBST(Node<K, V>* node, K min, K max)
{
  if(isNull(node))
  {
    return true;
  }
  if(node->m_key >= min && node->m_key <= max && isValidBST(node->m_child[LEFT],min,node->m_key-1) && isValidBST(node->m_child[RIGHT],node->m_key+1,max))
  {
    return true;
  }
  return false;
}

template<typename K, typename V>
bool ConcurrentTreeMap<K, V>::isValidTree()
{
	return(isValidBST(m_root, std::numeric_limits<K>::min(), std::numeric_limits<K>::max()));
}