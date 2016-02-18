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
#include <stdio.h>
#include "main.hpp"

static TCOD_semaphore_t sem=NULL;
static TCOD_mutex_t todoMutex=NULL;
static TCOD_mutex_t finishedMutex=NULL;
static TCODList<ThreadData *> todoList;
static TCODList<ThreadData *> finished;
static int jobId=0;
ThreadPool *threadPool=NULL;

int thread_pool_func(void *dat) {
	// wait for something to do
	do {
		TCODSystem::lockSemaphore(sem);
		// do some job
		if ( todoList.size() > 0 ) {
			// get some job from todo list
			TCODSystem::mutexIn(todoMutex);
			ThreadData *data = NULL;
			if ( todoList.size() > 0 ) {
				data = todoList.get(0);
				todoList.remove(data);
			}
			TCODSystem::mutexOut(todoMutex);
			if ( data && data->job != NULL ) {
				// do the job
				data->jobResult = data->job(data->jobData);
				TCODSystem::unlockSemaphore(data->sem);
				// and put the result in finished list
				TCODSystem::mutexIn(finishedMutex);
				finished.push(data);
				TCODSystem::mutexOut(finishedMutex);
			}
		}
	} while (true);
	return 0;
}

ThreadPool::ThreadPool() {
	static bool multithread=config.getBoolProperty("config.multithread");
	static int threadPoolSize=config.getIntProperty("config.threadPoolSize");
	nbCores = TCODSystem::getNumCores();
	int nbThreads=MAX(1,nbCores-1);
	printf ("Cores detected : %d\n",nbCores);
	if ( ! multithread ) {
		//printf ("Background threads disabled in config.txt\n");
		nbThreads=0;
	} else {
		if ( threadPoolSize > 0 ) {
			nbThreads = threadPoolSize;
			printf ("Background threads pool size (from config.txt) : %d\n",nbThreads);
		} else {
			printf ("Background threads pool size : %d\n",nbThreads);
		}
	}
	sem = TCODSystem::newSemaphore(0);
	todoMutex = TCODSystem::newMutex();
	finishedMutex = TCODSystem::newMutex();
	// start all the threads and put them in idle state
	for (int i=0; i < nbThreads; i++) {
		threads.push(TCODSystem::newThread(thread_pool_func,NULL));
	}

}

bool ThreadPool::isMultiThreadEnabled() {
	static bool multithread=config.getBoolProperty("config.multithread");
	return multithread;
}

int ThreadPool::addJob(thread_job_t job, void *jobData) {
	ThreadData *data=new ThreadData();
	data->job=job;
	data->jobData=jobData;
	data->sem = TCODSystem::newSemaphore(0);
	TCODSystem::mutexIn(todoMutex);
	data->id = jobId++;
	todoList.push(data);
	TCODSystem::mutexOut(todoMutex);
	TCODSystem::unlockSemaphore(sem);
	return data->id;
}

bool ThreadPool::isFinished(int jobId) {
	TCODSystem::mutexIn(finishedMutex);
	for ( ThreadData **it=finished.begin(); it != finished.end(); it++) {
		if ( (*it)->id == jobId ) {
			finished.remove(it);
			TCODSystem::mutexOut(finishedMutex);
			TCODSystem::deleteSemaphore((*it)->sem);
			delete (*it);
			return true;
		}
	}
	TCODSystem::mutexOut(finishedMutex);
	return false;
}

void ThreadPool::waitUntilFinished(int jobId) {
	if (isFinished(jobId)) return; // job already finished
	TCODSystem::mutexIn(todoMutex);
	for ( ThreadData **it=todoList.begin(); it != todoList.end(); it++) {
		if ( (*it)->id == jobId ) {
			// wait for job to finish
			TCODSystem::mutexOut(todoMutex);
			if ( threads.size() == 0 ) {
				// no thread ! do it yourself, pal!
				(*it)->job((*it)->jobData);
				TCODSystem::mutexIn(todoMutex);
				todoList.remove(it);
				TCODSystem::mutexOut(todoMutex);
				TCODSystem::deleteSemaphore((*it)->sem);
				delete (*it);
			} else {
				// wait for the thread to finished the job
				TCODSystem::lockSemaphore((*it)->sem);
				isFinished(jobId); // todo release resources
			}
			return;
		}
	}
	TCODSystem::mutexOut(todoMutex);
}


