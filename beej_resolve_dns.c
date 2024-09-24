/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   beej_resolve_dns.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/09/24 22:06:18 by mrizakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Program which simulates resolves a DNS address to an IP address
// Usage: launch with one parameter specifying the DNS address to resolve: ./a.out google.com





// getaddrinfo(argv[1], NULL, &hints, &res)
// argv[1] is dns address,
// NULl  is ?
// &hints - pointer to stuct addrinfo, provides config details
// &res - will point to the results

// Given node and service, which identify an Internet host and a
// service, getaddrinfo() returns one or more addrinfo structures,
// each of which contains an Internet address that can be specified
// in a call to bind(2) or connect(2). 
// struct addrinfo {
//                int              ai_flags;
//                int              ai_family;
//                int              ai_socktype;
//                int              ai_protocol;
//                socklen_t        ai_addrlen;
//                struct sockaddr *ai_addr;
//                char            *ai_canonname;
//                struct addrinfo *ai_next;
//            };


int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"usage: showip hostname\n");
        return 1;
    }
// make sure the struct is empty
    memset(&hints, 0, sizeof hints);
    
    hints.ai_family = AF_UNSPEC; // unspecified, don't care IPv4 or IPv6 - AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresses for %s:\n\n", argv[1]);

    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // the address is IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // the address is IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        //1.p->ai_family - This specifies the address family (either AF_INET for IPv4 or AF_INET6 for IPv6). It tells the function whether it should interpret the address as an IPv4 or IPv6 address.
        //p is &res, pointer to struct addrinfo
        //2. addr - This is a pointer to the binary representation of the IP address. Depending on the family (ai_family), this is either:
        //struct in_addr for IPv4 (AF_INET)
        //struct in6_addr for IPv6 (AF_INET6)
        //3. ipstr string(fixed-size) with the human-readable string
        //4. sizeof ipstr


        
        printf("  %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res); // free the linked list

    return 0;
}