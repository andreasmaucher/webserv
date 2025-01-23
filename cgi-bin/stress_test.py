#!/usr/bin/env python3
import http.client
import threading
import time
import sys

def make_request(thread_id, num_requests):
    success = 0
    failures = 0
    
    for i in range(num_requests):
        try:
            conn = http.client.HTTPConnection("localhost", 8080)
            conn.request("GET", "/cgi-bin/get.py")
            response = conn.getresponse()
            
            if response.status == 200:
                success += 1
                # Optional: read and discard response data
                response.read()
            else:
                failures += 1
                print(f"Thread {thread_id}, Request {i}: Failed with status {response.status}")
                print(f"Response: {response.read().decode()[:200]}...")  # Print first 200 chars
            
            conn.close()
        except Exception as e:
            failures += 1
            print(f"Thread {thread_id}, Request {i}: Exception: {str(e)}")
        
        # Small delay to avoid overwhelming the server
        time.sleep(0.1)
    
    return success, failures

def main():
    num_threads = 5      # Number of concurrent threads
    requests_per_thread = 20  # Number of requests per thread
    
    threads = []
    results = []
    
    print(f"Starting stress test with {num_threads} threads, {requests_per_thread} requests each")
    print(f"Total requests: {num_threads * requests_per_thread}")
    
    start_time = time.time()
    
    # Create and start threads
    for i in range(num_threads):
        thread = threading.Thread(target=lambda: results.append(make_request(i, requests_per_thread)))
        threads.append(thread)
        thread.start()
    
    # Wait for all threads to complete
    for thread in threads:
        thread.join()
    
    # Calculate total results
    total_success = sum(r[0] for r in results)
    total_failures = sum(r[1] for r in results)
    total_time = time.time() - start_time
    
    print("\nTest Results:")
    print(f"Total requests: {total_success + total_failures}")
    print(f"Successful: {total_success}")
    print(f"Failed: {total_failures}")
    print(f"Success rate: {(total_success/(total_success + total_failures))*100:.2f}%")
    print(f"Total time: {total_time:.2f} seconds")
    print(f"Requests per second: {(total_success + total_failures)/total_time:.2f}")

if __name__ == "__main__":
    main()