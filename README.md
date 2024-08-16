# Grading Request Server and Client Design

## Overview

This document describes the design and functionality of a server-client system for handling grading requests. The server manages requests through a multi-threaded approach, while the client interacts with the server asynchronously.

## Server Design

The server is responsible for receiving and processing grading requests. It maintains various data structures and employs multi-threading to handle requests efficiently.

### Data Structures

- **Listen Queue**: Queue for incoming requests.
- **Thread Queue**: Queue for requests that are being processed.
- **Status HashMap**: Tracks the status of each request.
  - **Status = 0**: Request is in Thread Queue.
  - **Status = 1**: Request is being served/processed.
  - **Status = 2**: Request has been served/processed.

### Threads and Thread Pools

- **Main Thread**: Manages the operation of the server.
  
- **Thread Pool for Receiving Files and Storing Grading Requests**:
  - Threads in this pool handle incoming requests.
  - For new requests: Receive the file, generate a unique Ticket-ID, and enqueue the request.
  - For status requests: Check the request status in the HashMap and respond accordingly.

- **Thread Pool for Processing Grading Requests**:
  - Threads in this pool pick requests from the Request Queue, process them, generate the output, and update the status in the HashMap.

### Workflow

1. **Request Handling**:
   - New grading requests are assigned a unique Ticket-ID based on the timestamp and are enqueued.
   - Status requests check the HashMap and respond with the current status of the request.

2. **Status Updates**:
   - **New Request Enqueued**: Status = 0
   - **Request Dequeued for Processing**: Status = 1
   - **Processing Complete**: Status = 2

3. **Client Status Check**:
   - If the request has been processed, the result is sent back to the client.
   - If the request is still in the queue, the server replies with "Request is in queue."

## Client Design

The client interacts with the server asynchronously to submit grading requests and check their status.

### Request Submission

- The client sends an asynchronous grading request to the server.
- The server responds with a Ticket-ID for the request.

### Status Check

- The client asynchronously sends a request to check the status of the grading request using the provided Ticket-ID.
- The server responds with the current status of the request or the results if available.

## Example Usage

### Server Workflow Example

1. **Client Request**:
   - Client sends a grading request.
   - Server assigns a Ticket-ID and returns it to the client.

2. **Status Check**:
   - Client queries the server with the Ticket-ID.
   - Server checks the HashMap and returns the status or result.

### Client Workflow Example

1. **Submitting a Request**:
   ```bash
   curl -X POST http://server-address/submit -d "file=grading-file" -o ticket-id.txt

