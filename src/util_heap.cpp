/*
* Copyright (c) 2010 Jice
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of Jice may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Jice ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Jice BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "main.hpp"

void BinaryHeap::siftDown() {
	// sift-down : move the first element of the heap down to its right place 
	int cur=0;
	int end = size()-1;
	int child=1;
	int toSwap=0;
	HeapObject **array=begin();
	while ( child <= end ) {
		HeapObject *curObj=array[cur];
		float curValue=curObj->getHeapValue();
		HeapObject *childObj=array[child];
		float childValue=childObj->getHeapValue();
		// swap with first son ?
		if ( childValue < curValue ) {
			toSwap=child;
			curValue=childValue;
		}
		if ( child < end ) {
			// swap with second son ? 
			HeapObject *childObj2=array[child+1];
			float child2Value=childObj2->getHeapValue();
			if ( child2Value < curValue ) {
				toSwap=child+1;
			}
		}
		if ( toSwap != cur ) {
			// get down one level
			HeapObject *tmp = array[toSwap];
			array[toSwap]=array[cur];
			array[cur]=tmp;
			cur=toSwap;
		} else return;
		child=cur*2+1;
	}
}

void BinaryHeap::siftUp() {
	// sift-up : move the last element of the heap up to its right place 
	int end = size()-1;
	int child=end;
	HeapObject **array=begin();
	while ( child > 0 ) {
		HeapObject *childObj=array[child];
		float childValue=childObj->getHeapValue();
		int parent = (child-1)/2;
		HeapObject *parentObj=array[parent];
		float parentValue=parentObj->getHeapValue();
		if ( parentValue > childValue ) {
			// get up one level 
			HeapObject *tmp = array[child];
			array[child]=array[parent];
			array[parent]=tmp;
			child=parent;
		} else return;
	}
}

void BinaryHeap::reorder(HeapObject *obj) {
	HeapObject **array=begin();
	int cur=0;
	// find the object in the heap (slow...)
	HeapObject **it=begin();
	while ( it != end() && *it != obj ) {
		cur++;
		it++;
	}
	if ( it == end() ) return; // not found
	float value=obj->getHeapValue();
	if ( cur > 0 ) {
		int parent=(cur-1)/2;
		// compare to its parent
		HeapObject *parentObj=array[parent];
		if (value < parentObj->getHeapValue()) {
			// smaller. bubble it up
			while ( cur > 0 && value < parentObj->getHeapValue() ) {
				// swap with parent
				HeapObject *tmp=parentObj;
				array[parent]=obj;
				array[cur] = tmp;
				cur=parent;
				if ( cur > 0 ) {
					parent=(cur-1)/2;
					parentObj=array[parent];
				}
			}
			return;
		} 
	}
	// compare to its sons
	while ( cur*2+1 < size() ) {
		int child=cur*2+1;	
		HeapObject *childObj=array[child];
		int toSwap=cur;
		float swapValue=value;
		if ( childObj->getHeapValue() < value ) {
			// swap with son1 ?
			toSwap=child;
			swapValue=childObj->getHeapValue();
		}
		int child2 = child+1;
		if ( child2 < size() ) {
			HeapObject *child2Obj=array[child2];
			if ( child2Obj->getHeapValue() < swapValue) {
				// swap with son2
				toSwap=child2;
			}
		}
		if ( toSwap != cur ) {
			// bigger. bubble it down
			HeapObject *tmp = array[toSwap];
			array[toSwap] = obj;
			array[cur] = tmp;
			cur=toSwap;			
		} else return;
	}	
}

// add an object in the heap so that the heap root always contains the minimum value
void BinaryHeap::add(HeapObject *obj) {
	// append the new value to the end of the heap
	push(obj); 
	// bubble the value up to its real position 
	siftUp();
}

// pop the object with minimum value from the heap
HeapObject *BinaryHeap::popMin() {
	// return the first value of the heap (minimum score) 
	HeapObject **array=begin();
	int end=size()-1;
	HeapObject *obj=array[0];
	// move the last element at first position (heap root) 
	array[0] = array[end];
	pop();
	// and bubble it down to its real position 
	siftDown();
	return obj;
}
