/*
 * TaskQueue.cpp
 *
 *  Created on: May 21, 2016
 *      Author: z.j
 */
 
#include "TaskQueue.h"
#include "Task.h"

#include <iostream>

using namespace cm;
using namespace std;
 
// ------------------------------------
TaskQueue::TaskQueue() 
: m_firstTask(0), m_lastTask(0), m_length(0)
{
     
}
 
// ------------------------------------
TaskQueue::~TaskQueue() {
    while (m_firstTask != 0) {
        Task* temp = m_firstTask->next;
        delete m_firstTask;
        m_firstTask = temp;
    }
    
    m_lastTask = 0;
}

// ------------------------------------
int TaskQueue::executeTask() {          
    m_lock.lock();
    
    if (m_length == 0) {
        // should not happen
        cout << "Empty queue!" << endl;       
        m_lock.unlock();
        return TRC_EMPTY;
    }
    
    if (m_firstTask != 0) {
        Task* task = m_firstTask;
        m_firstTask = m_firstTask->next;
        if (m_firstTask == 0) {
            m_lastTask = 0;
        }
        m_length--;
        
        m_lock.unlock();
        
        int result = task->execute();
        delete task;
        return result;
    } else {
        // should not happen
        cout << "No available task!" << endl;
        m_lock.unlock();
        return TRC_EMPTY;
    }
}
 
// ------------------------------------
bool TaskQueue::addTask(Task* theTask) {
    if (theTask == 0) {
        return false;
    }
    
    m_lock.lock();
    
    if (m_lastTask == 0) {
        // the queue is empty
        m_firstTask = theTask;
        m_lastTask = theTask;
    } else {
        // the queue is not empty
        m_lastTask->next = theTask;
        m_lastTask = theTask;
    }
    
    m_length++;
    
    m_lock.unlock();
    
    return true;
}
 