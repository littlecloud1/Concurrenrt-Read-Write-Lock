//#include <stdio.h>
#include "stdafx.h"
#include <thread>
#include <iostream>
//#include "RWlock.h"

#include <cstddef>
//#include <thread>
#include <atomic>
#include <vector>

using namespace std;



class ReaderTree {
	//   inner class Node
	class Node {
	private:
		Node * parent; // parent node of this node
					   //atomic<int> childCount; // total count of all children
		bool threadSense; // identify if the thread has left or not
						  //ReaderTree& tree;// a reference of current tree barrier

	public:
		Node(Node* myParent) {
			this->parent = myParent;
			this->threadSense = false;

		}
		~Node() {
			if (threadSense) {
				//free(parent);
			}
		}
		// wait for safe to leave
		void await() {
			if (parent != NULL) {
				while (parent->threadSense != true) {};
			}
		}

		void setThreadSense() {
			this->threadSense = true;
		}

		bool getThreadSense() {
			return threadSense;
		}

	};
	atomic<bool> sense; // tell the reader in this tree can start to leave.
private:
	atomic<int> size;  // current size of the tree
	vector<Node*> node; // queue for reader node
						//int radix;  // Each node's number of children
	atomic<int> nodes;   // total readers in the tree
						 //bool threadSense;
	Node* root;

public:
	//constructor
	ReaderTree() {
		size.store(0, memory_order_relaxed);
		this->nodes = 0;
		//int depth = 0;
		// 1 layer tree structure
		//while (size > 1){
		//	depth++;
		//	this->size = size;
		//	this->radix = size-1;
		//}
		this->node.push_back(new Node(NULL));
		//build(root, depth);
		this->sense.store(false, memory_order_relaxed);
		//this->threadSense = !this->sense.load(memory_order_relaxed);
	}


	//recursive tree constructor for reader
	/*void build(Node* parent, int depth){
	if(depth == 0){
	Node* tmp = new Node(parent, 0, this->threadSense, this);
	this->node.push(tmp);
	nodes++;
	}
	else {
	Node* myNode = new Node(parent, this->radix, this->threadSense, this);
	for (int i =0; i < this->radix; i++){
	build(myNode, depth -1);
	}
	}
	}*/

	~ReaderTree() {
		//	for (int i = 0; i < nodes; i++) {
		//	node[i]->~Node();
		//free(node[i]);
		//}
	}

	//add a node, return its id.
	int addReader() {
		Node* tmp = new Node(node[0]);
		this->node.push_back(tmp);
		this->nodes.fetch_add(1, memory_order_relaxed);
this->size.fetch_add(1, memory_order_relaxed);
return this->nodes.load(memory_order_relaxed);
	}

	//Reader await.  after finishing await, it leaves the ReaderTree
	void await(int id) {
		this->node[id]->await();
		this->node[id]->setThreadSense();
		this->size.fetch_sub(1, memory_order_acquire);
	}


	bool isEmpty() {
		if (this->size.load(memory_order_relaxed) == 0) return true;
		else return false;
	}

	void setSense() {
		//	sense.store(true, memory_order_release);
		this->node[0]->setThreadSense();
	}


};



class RWnode {                    // this class defines both writer node and reader tree node
public:
	bool iswriter = false;
	ReaderTree* rtree = NULL;
	int reader_id = 0;
	int writer_id = 0;
public:
	RWnode() {
	}
	void constructtree() {
		rtree = new ReaderTree();
	}
	void addreader() {
		rtree->addReader();
	}
	bool tree_empty() {
		if (rtree->isEmpty()) {
			return true;
		}
		else {
			return false;
		}
	}

};



class RWQueue {           // this class defines the R/W queue

	class QueueNode {
	public:
		QueueNode() {
			nextitem = NULL;
			item = NULL;
		}
		QueueNode(RWnode* outitem) {

			item = outitem;
		}
		RWnode* getItem() {
			return item;
		}

		QueueNode* getNext() {
			return nextitem;
		}

		void setNext(QueueNode* next) {
			nextitem.store(next,memory_order_relaxed);
		}
	public:
		atomic<QueueNode*> nextitem;
		RWnode * item;
	};

public:
	RWQueue() {
		first = new QueueNode();
		last = new QueueNode();
		size.store(0, memory_order_relaxed);
	}

	void enqueue(RWnode* o) {
		QueueNode* newnode = new QueueNode(o);
		
		if (first.load(memory_order_relaxed)->getItem() == NULL ) {
			first.load(memory_order_relaxed)->setNext(newnode);
			first.store(first.load(memory_order_relaxed)->getNext(), memory_order_relaxed);
			last.load(memory_order_relaxed)->setNext(newnode);
			last.store(last.load(memory_order_relaxed)->getNext(), memory_order_relaxed);
			size.fetch_add(1, memory_order_relaxed);
		}
		else {
			last.load(memory_order_relaxed)->setNext(newnode);
			
			last.store(last.load(memory_order_relaxed)->getNext(), memory_order_relaxed);
			size.fetch_add(1, memory_order_relaxed);
		}
	}

	RWnode * dequeue() {
		if (first == last) {                           //only one item in the queue, need to change the last pointer
			QueueNode* nodewithitem = first.load(memory_order_relaxed);
			RWnode * objecttoreturn = nodewithitem->getItem();
			size.fetch_sub(1, memory_order_release);
			QueueNode* temp = new QueueNode();
			first.store(temp, memory_order_relaxed);
			last.store(temp, memory_order_relaxed);
			free(nodewithitem);                         //free memory
			return objecttoreturn;

		}
		else {          //multiple items, do not need to consider last pointer
			QueueNode* nodewithitem = first.load(memory_order_relaxed);
			RWnode * objecttoreturn = nodewithitem->getItem();
			size.fetch_sub(1, memory_order_release);

			first.store(first.load(memory_order_relaxed)->getNext(), memory_order_relaxed);
			free(nodewithitem);                      //free memory
			return objecttoreturn;
		}
	}

	RWnode* getfront() {                  // get the first item
		if (first.load(memory_order_relaxed) != NULL) {
			QueueNode* nodewithitem = first.load(memory_order_relaxed);
			
			return nodewithitem->getItem();
		}
		else {
			cout << "should not occur: empty queue, can not get front";
		}
	}
	RWnode* getlast() {
		if (last.load(memory_order_relaxed) != NULL) {
			QueueNode* nodewithitem = last.load(memory_order_relaxed);
			
			return nodewithitem->getItem();
		}
		cout << "should not occur: can not use get last beacuse queue is empty";
	}
	bool isfront(RWnode* mynode) {           // check if a reader/writer is at front so that we can do it
		if (first.load(memory_order_relaxed)->getItem() == NULL){
			
			return false;
		}
		else
		{
			if (first.load(memory_order_relaxed)->getItem() == mynode) {
				
				return true;
			}
			else {
				
				return false;
			}
		}
		
	}

	int getsize() {
		return size.load(memory_order_relaxed);
	}

	bool isEmpty() {
		return size.load(memory_order_relaxed) == 0;
	}
protected:
	atomic<QueueNode*> first;
	atomic<QueueNode*> last;
	atomic<int> size;

};


class RWlock {
protected:
	RWQueue * myqueue;
	
public:
	RWlock() {
		
		myqueue = new RWQueue();
	}
	void read_lock(RWnode* myreader) {

		if (myqueue->isEmpty()) {                  // is the first node
			myreader->constructtree();
			
			myreader->addreader();
			myqueue->enqueue(myreader);
		}
		else {                                // there are nodes in the queue
			if (myqueue->getlast()->iswriter) {   // find out the last one in the queue is a writer
				myreader->constructtree();       // need a new tree
												 
				myreader->addreader();
				myqueue->enqueue(myreader);
			}
			else {                                            // the last one is a reader tree. 
															  
				myqueue->getlast()->addreader();
			}
		}
		
		while (!myqueue->isfront(myreader)) {}     // is not front, can not do the job, spin
	}
	void write_lock(RWnode* mywriter) {
		
		myqueue->enqueue(mywriter);
		while (!myqueue->isfront(mywriter)) {}
	}
	void read_unlock() {
		//if (myqueue->getfront()->tree_empty()) { //!!!!!!!!!!!!!!!!!!!!!!!!!! test only!!!!!!!
			myqueue->dequeue();
		//}    //test only !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}
	void write_unlock() {
		myqueue->dequeue();
	}
};




RWlock * mylock;

int var = 0;


void reader(void *arg)
{
	int* id = (int*)arg;
	RWnode* myreader = new RWnode();
	printf("\nReader %d constructed ", &id);
	myreader->reader_id = *id;
	mylock->read_lock(myreader);
	printf("\nReader is locked ");
	
	int newvar = var;
	cout << "read var:" << newvar << endl;
	mylock->read_unlock();
	printf("\nReader is unlocked ");

}

void writer(void *arg)
{
	int* id = (int*)arg;
	RWnode * mywriter = new RWnode();
	mywriter->iswriter = true;
	mywriter->writer_id = *id;
	mylock->write_lock(mywriter);
	printf("\nWriter is locked ");
	var += 1;
	cout << "after write var:" << var << endl;
	mylock->write_unlock();
	printf("\nWriter is unlocked ");

}
int main()
{

	int i = 1;
	int y = 1;
	mylock = new RWlock();
	std::thread R1(reader, &i);
	R1.join();
	i++;
	//std::thread R2(reader, &i);
	//R2.join();
	std::thread W1(writer, &y);
	W1.join();
	y++;
	std::thread W2(writer, &y);
	W2.join();
	std::thread R3(reader, &i);
	R3.join();

	return 0;
}

