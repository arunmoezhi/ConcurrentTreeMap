#pragma once
#include<limits>
#include"Node.h"
template<typename K, typename V>
class TreeMap
{
private:
	Node<K, V>* m_root;

public:
	TreeMap();

	V lookup(K key);
	//bool insert(K key, V value);
	//bool remove(K key);
};

template<typename K, typename V>
TreeMap<K, V>::TreeMap()
{
	m_root = new Node<K,V>(std::numeric_limits<K>::min(),std::numeric_limits<V>::min());
}

template<typename K, typename V>
V TreeMap<K, V>::lookup(K key)
{
	return m_root->m_value;
}