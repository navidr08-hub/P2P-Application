/*server.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
struct pdu
{
    char type;
    char data[255];
};

struct RegisteredPeers
{
    char type;
    char peername[255];
    char contentname[255];
    char address[255];
};

struct RegisteredPeers obj[10];

// Given peer_name and content_name return 0 if there is no such registered content, 1 otherwise.
int entryExists(size_t numElements, const char *peer_name, const char *content_name)
{
    int i;
    for (i = 0; i < numElements; ++i)
    {
        if (strcmp(obj[i].peername, peer_name) == 0 && strcmp(obj[i].contentname, content_name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// Given peer_name and content_name return the IPv4 address of the peer on the network.
// Returns NULL if no such peer is on the network.
const char *findAddress(size_t numElements, const char *peer_name, const char *content_name)
{
    int i;
    for (i = 0; i < 10; i++)
    {
        printf("%s \n", obj[i].peername);
        printf("%s \n", obj[i].contentname);
        if (strcmp(obj[i].peername, peer_name) == 0 && strcmp(obj[i].contentname, content_name) == 0)
        {
            return obj[i].address;
        }
    }
    return NULL;
}

// Given peer_name, content_name and ip address, register a new peer.
void addData(int index, const char *s, const char *f, const char *c)
{
    // Check if the index is within bounds
    if (index >= 0 && index < 10)
    {
        strncpy(obj[index].peername, s, sizeof(obj[index].peername) - 1);
        strncpy(obj[index].contentname, f, sizeof(obj[index].contentname) - 1);
        strncpy(obj[index].address, c, sizeof(obj[index].address) - 1);

        // Ensure null-termination of strings
        obj[index].peername[sizeof(obj[index].peername) - 1] = '\0';
        obj[index].contentname[sizeof(obj[index].contentname) - 1] = '\0';
        obj[index].address[sizeof(obj[index].address) - 1] = '\0';
    }
    else
    {
        fprintf(stderr, "Index out of bounds\n");
    }
}

// Remove peer given peer_name and content_name.
// Return 1 if the peer was removed, 0 otherwise.
int removeEntry(const char *peer_name, const char *content_name)
{
    int i;
    for (i = 0; i < 10; ++i)
    {
        if (!isIndexEmpty(i) && strcmp(obj[i].peername, peer_name) == 0 &&
            strcmp(obj[i].contentname, content_name) == 0)
        {
            // Clear the entry when the specified names match
            memset(&obj[i], 0, sizeof(obj[i]));
            printf("Entry removed successfully.\n");
            return 1;
        }
    }
    printf("Entry not found.\n");
    return 0;
}

// Return 1 if index in RegisteredPeers array is empty/free, 0 otherwise.
int isIndexEmpty(size_t index)
{
    // Check if the peername field is an empty string or some default value
    return obj[index].peername[0] == '\0';
}

// Print the list of registered contents along with their peer_name and address.
void printContentServers(size_t numElements)
{
    int i;
    for (i = 0; i < numElements; i++)
    {
        if (!isIndexEmpty(i))
        {
            printf("Peer Name: %s \n", obj[i].peername);
            printf("Content Name: %s \n", obj[i].contentname);
            printf("Content Address: %s \n", obj[i].address);
            printf("\n");
        }
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in fsin; /* the from address of a client */
    char *service = "3000";  /* service name or port number  */
    int n;            /* server socket        */
    int alen;               /* from-address length      */
    struct sockaddr_in sin; /* an Internet endpoint address         */
    int s;            /* socket descriptor    */
    int registeredServersIndex = 0;

    struct RegisteredPeers spdu;
    struct pdu response;

    switch (argc)
    {
    case 1:
        break;
    case 2:
        service = argv[1];
        break;
    default:
        fprintf(stderr, "usage: time_server [host [port]]\n");
    }

    // Initialize internet endpoint address
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    /* Map service name to port number */
    sin.sin_port = htons((u_short)atoi(service));

    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        fprintf(stderr, "can't create socket\n");
        exit(1);
    }

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        fprintf(stderr, "can't bind to %s port\n", service);

    while (1)
    {
        // Receive incoming connection and store data in spdu
        alen = sizeof(fsin);
        if ((n = recvfrom(s, (void *)&spdu, sizeof(spdu), 0,
                          (struct sockaddr *)&fsin, &alen)) < 0)
            fprintf(stderr, "recvfrom error\n");

        if (spdu.type == 'S')
        {
            printf("Content Download request sent by content client \n");
            printf("-- Peer name -- here is %s\n", spdu.peername);
            printf("-- Content name -- here is %s\n", spdu.contentname);
            const char *foundAddress = findAddress(10, spdu.peername, spdu.contentname);

            /* Content address found */
            if (foundAddress != NULL)
            {
                response.type = 'S';
                strncpy(response.data, foundAddress, 255);
                printf("Content Client requested content that exists\n");
            }
            /* Content address not found */
            else
            {
                printf("Content Client requested content that DOES NOT exist\n");
                printf("These are current registered servers: \n");
                printContentServers(10);
            }
            (void)sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&fsin, sizeof(fsin));
        }
        else if (spdu.type == 'R')
        {
            printf("Content Registration request sent by aspiring content server \n");
            printf("Type here is R \n");

            if (entryExists(10, spdu.peername, spdu.contentname))
            {
                response.type = 'E';
                printf("Peer name and content name already registered. Try with different names. \n");
                (void)sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
            else
            {
                response.type = 'A';
                /*response.data =  "Success";*/
                addData(registeredServersIndex, spdu.peername, spdu.contentname, spdu.address);
                printf("PDU SUCCESSFULLY REGISTERED!!! \n");
                printContentServers(10);
                registeredServersIndex++;
                (void)sendto(s, &response, strlen(response.data) + 1, 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
        }
        else if (spdu.type == 'O')
        {
            printf("Content List request sent in by peer \n");

            struct pdu response;
            response.type = 'O';
            memset(response.data, 0, sizeof(response.data));

            int offset = 0;
            int i;
            for (i = 0; i < 10; ++i)
            {
                if (!isIndexEmpty(i))
                {

                    snprintf(response.data + offset, sizeof(response.data) - offset,
                             "Peer Name: %s\nContent Name: %s\nContent Address: %s\n\n",
                             obj[i].peername, obj[i].contentname, obj[i].address);
                    offset = strlen(response.data);
                }
            }

            (void)sendto(s, &response, sizeof(response), 0, (struct sockaddr *)&fsin, sizeof(fsin));
        }

        else if (spdu.type == 'T')
        {
            printf("Content de-register request sent in \n");
            printf("Peername:  %s \n", spdu.peername);
            printf("Contentname to remove is %s \n", spdu.contentname);

            if (removeEntry(spdu.peername, spdu.contentname))
            {
                printf("Successfully removed peer \n");
                response.type = 'A';
                (void)sendto(s, &response, sizeof(response), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
            else
            {
                printf("Error removing peer \n");
                response.type = 'E';
                (void)sendto(s, &response, sizeof(response), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
        }
    }
}
