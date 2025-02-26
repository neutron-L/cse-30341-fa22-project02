# Project 02: Message Queue

This is [Project 02] of [CSE.30341.FA22].

## Students

1. Domer McDomerson (dmcdomer@nd.edu)
2. Belle Fleur (bfleur@nd.edu)

## Brainstorming

The following are questions that should help you in thinking about how to
approach implementing [Project 02].  For this project, responses to these
brainstorming questions **are not required**.

### Request

1. What data must be allocated and deallocated for each `Request` structure?

2. What does a valid **HTTP** request look like?

### Queue

1. What data must be allocated and deallocated for each `Queue` structure?

2. How will you implement **mutual exclusion**?

3. How will you implement **signaling**?
    
4. What are the **critical sections**?

### Client

1. What data must be allocated and deallocated for each `MessageQueue`
   structure?

2. What should happen when the user **publishes** a message?

3. What should happen when the user **retrieves** a message?

4. What should happen when the user **subscribes** to a topic?

5. What should happen when the user **unsubscribes** to a topic?
    
6. What needs to happen when the user **starts** the `MessageQueue`?

7. What needs to happen when the user **stops** the `MessageQueue`?

8. How many internal **threads** are required?

9. What is the purpose of each internal **thread**?

10. What `MessageQueue` attribute needs to be **protected** from **concurrent**
    access?
    
## Demonstration

> Link to **video demonstration** of **chat application**.

## Errata

> Describe any known errors, bugs, or deviations from the requirements.

[Project 02]:       https://www3.nd.edu/~pbui/teaching/cse.30341.fa22/project02.html
[CSE.30341.FA22]:   https://www3.nd.edu/~pbui/teaching/cse.30341.fa22/

## Problem
每一次发送的请求，server都会返回一个响应，这个响应的格式和retrieve请求的返回的格式是一样的，在incoming里面无法区分.
修改后，除了get之外的请求，由pusher接收响应，反之通过条件变量通知puller接收响应
但server似乎有点问题，无法有时候无法接收到client发送的消息……
或者可以让pusher把retrieve的请求的序号通过队列保存，由puller取出，如果是GET请求的响应则保留
