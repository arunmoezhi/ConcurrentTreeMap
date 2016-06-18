#pragma once
#include<limits>
#include"Node.h"
#define LEFT 0
#define RIGHT 1
template<typename K, typename V>
class TreeMap
{
private:
	Node<K, V>* m_root;
private:
	inline bool CAS(Node<K, V>* parent, int which, Node<K, V>* oldChild, Node<K, V>* newChild);
	inline void seek(K key, Node<K, V>* parent, Node<K, V>* node, Node<K, V>* injectionPoint);
	Node<K, V>* getAddress(Node<K, V>* p);
	inline bool isNull(Node<K, V>* p);
	inline bool isLocked(Node<K, V>* p);
	inline Node<K, V>* setLock(Node<K, V>* p);
	inline Node<K, V>* unsetLock(Node<K, V>* p);
	inline Node<K, V>* setNull(Node<K, V>* p);
	inline Node<K, V>* newLeafNode(K key, V value);
	inline bool lockEdge(Node<K, V>* parent, Node<K, V>* oldChild, int which, bool n);
	inline void unlockEdge(Node<K, V>* parent, int which);

public:
	TreeMap();

	V lookup(const K key);
	bool insert(K key, V value);
	//bool remove(K key);
};

template<typename K, typename V>
TreeMap<K, V>::TreeMap()
{
	m_root = newLeafNode(std::numeric_limits<K>::min(), std::numeric_limits<V>::min());
	m_root->m_child[RIGHT] = newLeafNode(std::numeric_limits<K>::max(), std::numeric_limits<V>::max());
}

template<typename K, typename V>
Node<K, V>* TreeMap<K, V>::getAddress(Node<K, V>* p)
{
	return (Node<K, V>*)((uintptr_t)p & ~((uintptr_t)3));
}
template<typename K, typename V>
inline bool TreeMap<K, V>::isNull(Node<K, V>* p)
{
	return ((uintptr_t)p & 2) != 0;
}

template<typename K, typename V>
inline bool TreeMap<K, V>::isLocked(Node<K, V>* p)
{
	return ((uintptr_t)p & 1) != 0;
}

template<typename K, typename V>
inline Node<K, V>* TreeMap<K, V>::setLock(Node<K, V>* p)
{
	return (Node<K, V>*) ((uintptr_t)p | 1);
}

template<typename K, typename V>
inline Node<K, V>* TreeMap<K, V>::unsetLock(Node<K, V>* p)
{
	return (Node<K, V>*) ((uintptr_t)p & ~((uintptr_t)1));
}

template<typename K, typename V>
inline Node<K, V>* TreeMap<K, V>::setNull(Node<K, V>* p)
{
	return (Node<K, V>*) ((uintptr_t)p | 2);
}

template<typename K, typename V>
inline bool TreeMap<K, V>::CAS(Node<K, V>* parent, int which, Node<K, V>* oldChild, Node<K, V>* newChild)
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
inline Node<K, V>* TreeMap<K, V>::newLeafNode(K key, V value)
{
	return new Node<K, V>(key, value, setNull(nullptr), setNull(nullptr));
}

template<typename K, typename V>
inline bool lockEdge(Node<K, V>* parent, Node<K, V>* oldChild, int which, bool n)
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
inline void unlockEdge(Node<K, V>* parent, int which)
{
	parent->m_child[which] = unsetLock(parent->m_child[which]);
}

template<typename K, typename V>
inline void TreeMap<K, V>::seek(K key, Node<K, V>* parent, Node<K, V>* node, Node<K, V>* injectionPoint)
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
		parent = prev[index];
		node = curr[index];
		injectionPoint = address[index];
		return;
	}
}

template<typename K, typename V>
V TreeMap<K, V>::lookup(const K key)
{
	Node<K, V> parent;
	Node<K, V> node;
	Node<K, V> injectionPoint;

	seek(key, &parent, &node, &injectionPoint);
	if (node.m_key == key)
	{
		return node.m_value;
	}
	else
	{
		return std::numeric_limits<V>::max();
	}
}

template<typename K, typename V>
inline bool TreeMap<K, V>::insert(K key, V value)
{
	Node<K, V> parent;
	Node<K, V> node;
	Node<K, V> injectionPoint;
	K nKey;
	int which;
	bool result;
	while (true)
	{
		seek(key, &parent, &node, &injectionPoint);
		nKey = node.m_key;
		if (nKey == key)
		{
			return false;
		}
		//create a new node and initialize its fields
		Node<K, V>* newNode = newLeafNode(key, value);
		which = key < nKey ? LEFT : RIGHT;
		result = CAS(&node, which, setNull(&injectionPoint), newNode);
		if (result)
		{
			return true;
		}
	}
}
