#include<iostream>
#include<stdlib.h>
#include "TreeMap.h"
#include <new>

int main()
{
	std::cout << "hello world" << std::endl;
	//Node<int>* node = new Node<int>(5,nullptr,nullptr);
	//std::cout << node->getKey() << " "  << static_cast<void*>(node->getChild(0)) << std::endl;
	TreeMap<int, int> map;
	map.insert(5,50);
	std::cout << map.lookup(5) << std::endl;
	std::cout << map.remove(5) << std::endl;
	std::cout << map.remove(5) << std::endl;
	//TreeMap<float, float> mapf;
	//std::cout << mapf.lookup(2) << std::endl;
	return 0;
}