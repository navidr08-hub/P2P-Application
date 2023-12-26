/*client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <errno.h>

#define BUFSIZE 64

/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */
struct pdu
{
    char type;
    char data[255];
};

struct RegistrationPDU
{
    char type;
    char peername[255];
    char contentname[255];
    char address[255];
};

struct AddressFinderPDU
{
    char type;
    char peername[255];
    char contentname[255];
};

struct DPDU
{
    char type;
    char contentname[255];
};

char *trimwhitespace(char *str)
{
    char *end;
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';
    return str;
}

int main(int argc, char *argv[])
{
    char *host = "localhost"; /* host to use if none supplied */
    char *service = "3000";
    struct hostent *phe;    /* pointer to host information entry    */
    struct sockaddr_in sin; /* an Internet endpoint address     */
    int s, n;         /* socket descriptor and socket type    */

    switch (argc)
    {
    case 1:
        host = "localhost";
        break;
    case 3:
        service = argv[2];
        /* FALL THROUGH */
    case 2:
        host = argv[1];
        break;
    default:
        fprintf(stderr, "usage: UDPtime [host [port]]\n");
        exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    /* Map service name to port number */
    sin.sin_port = htons((u_short)atoi(service));

    if (phe = gethostbyname(host))
    {
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
    {
        fprintf(stderr, "Can't get host entry \n");
        exit(1);
    }

    /* Allocate a socket */
    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        fprintf(stderr, "Can't create socket \n");
        exit(1);
    }

    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        fprintf(stderr, "Can't connect to %s %s \n", host, service);
        exit(1);
    }
    struct pdu spdu;
    struct pdu opdu;
    struct RegistrationPDU reg_pdu;
    struct AddressFinderPDU add_find_pdu;
    struct RegistrationPDU client_reg_pdu;
    struct pdu response;
    struct DPDU dpdu;
    struct pdu cpdu;
    struct pdu cpdu_res;
    struct RegistrationPDU tpdu;

    // Set the type to FILENAME PDU

    while (1)
    {
        int user_choice;
        printf("Welcome to the P2P application! \n");
        printf("Choose your desired role/option from the options listed below: \n");
        printf("1. Content Server \n");
        printf("--------------------------------------------------------------- \n");
        printf("2. Content Client \n");
        printf("--------------------------------------------------------------- \n");
        printf("3. View Content Listing \n");
        printf("--------------------------------------------------------------- \n");
        scanf("%d", &user_choice);

        /* User wants to be content server */
        if (user_choice == 1)
        {

            /* Create a TCP Socket for content download */
            int stcp;
            struct sockaddr_in reg_addr;
            int alen;
            stcp = socket(AF_INET, SOCK_STREAM, 0);
            reg_addr.sin_family = AF_INET;
            reg_addr.sin_port = htons(0);
            reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(stcp, (struct sockaddr *)&reg_addr, sizeof(reg_addr)) < 0)
            {
                perror("bind");
                exit(EXIT_FAILURE);
            }

            alen = sizeof(struct sockaddr_in);
            int check = getsockname(stcp, (struct sockaddr *)&reg_addr, &alen);

            char ip_address[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(reg_addr.sin_addr), ip_address, INET_ADDRSTRLEN);
            printf("Address Port: %d\n", ntohs(reg_addr.sin_port));
            printf("Address IP Address: %s\n", ip_address);

            char server_ip[INET_ADDRSTRLEN];
            char full_address[255];

            reg_pdu.type = 'R';
            printf("Enter your peername: \n");
            scanf("%s", reg_pdu.peername);
            printf("Enter your content name: \n");
            scanf("%s", reg_pdu.contentname);
            printf("Enter your server IP Address: \n");
            scanf("%s", server_ip);

            snprintf(full_address, sizeof(full_address), "%s:%d", server_ip, ntohs(reg_addr.sin_port));

            printf("Full Address here is %s\n", full_address);
            strcpy(reg_pdu.address, full_address);

            write(s, &reg_pdu, 765);

            /* Get response from index server */
            n = read(s, &response, sizeof(response));
            if (n < 0)
            {
                fprintf(stderr, "Read failed\n");
            }

            if (response.type == 'A')
            {
                printf("Index Server successfully registered your content!\n");
                if (listen(stcp, 5) < 0)
                {
                    perror("listen");
                    exit(EXIT_FAILURE);
                }

                // Print information about the listening socket
                printf("Listening on port %d for incoming connections...\n", ntohs(reg_addr.sin_port));

                fd_set rfds, afds;
                int new_sd;
                char buf[255];
                struct sockaddr_in client;

                FD_ZERO(&afds);
                FD_SET(stcp, &afds);
                FD_SET(0, &afds);

                while (1)
                {
                    memcpy(&rfds, &afds, sizeof(rfds));

                    if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0)
                    {
                        perror("select");
                        exit(EXIT_FAILURE);
                    }
                    if (FD_ISSET(0, &rfds))
                    {
                        char userInput[BUFSIZE];
                        fgets(userInput, BUFSIZE, stdin);
                        userInput[strcspn(userInput, "\n")] = '\0'; // Remove newline character

                        if (strcmp(userInput, "q") == 0)
                        {
                            tpdu.type = 'T';

                            strcpy(tpdu.peername, reg_pdu.peername);
                            strcpy(tpdu.contentname, reg_pdu.contentname);
                            printf("Peername trying to remove here is %s \n", tpdu.peername);
                            printf("Contentname trying to remove here is %s \n", tpdu.contentname);
                            write(s, &tpdu, 510);

                            n = read(s, &response, sizeof(response));
                            if (n < 0)
                            {
                                fprintf(stderr, "Read failed\n");
                            }

                            if (response.type == 'A')
                            {
                                printf("Closed connection and terminated TCP connection.\n");
                                close(stcp);
                            }
                            else
                            {
                                printf("Error degistering content. \n");
                                close(stcp);
                            }

                            // Handle any other cleanup or termination steps here if needed
                            break; // Exit the loop upon 'q' input
                        }
                    }

                    if (FD_ISSET(stcp, &rfds))
                    {
                        new_sd = accept(stcp, (struct sockaddr *)&client, &alen);
                        if (new_sd != -1)
                        {
                            printf("Got new content client!\n");

                            char sbuf[255];
                            int n;

                            // Read only when the connection is established
                            struct DPDU serverResponse;
                            n = read(new_sd, &serverResponse, 255);
                            printf("serverResponse.type here is %s\n", &serverResponse.contentname);

                            if (serverResponse.type == 'D')
                            {
                                printf("got in d if block\n");
                                cpdu.type = 'C';
                                /*strcpy(cpdu.data, "rgwerg wreergwrr r");*/

                                char *trimmed;
                                trimmed = trimwhitespace(serverResponse.contentname);

                                FILE *fp = fopen(trimmed, "r");
                                if (fp == NULL)
                                {
                                    printf("-> Client requested file that does not exist \n");
                                    write(stcp, "NO FILE", n);
                                    close(stcp);
                                    return 1;
                                }
                                else
                                {
                                    size_t bytesRead;
                                    while ((n = fread(sbuf, 1, 255, fp)) > 0)
                                    {
                                        printf("inside actual while loop");
                                        strcpy(cpdu.data, sbuf);
                                        write(new_sd, &cpdu, 255);
                                        printf("AFTER inside actual while loop");
                                    }
                                    fclose(fp);
                                    close(stcp);
                                }
                                return 0;
                            }
                        }
                        else
                        {
                            perror("accept");
                            // Handle the error
                        }
                    }
                }
            }
            else if (response.type = 'E')
            {
                printf("Peer name and content name already exist.\n");
                printf("Please choose another peer name.\n");
            }
        }

        /* User wants to be content client */
        else if (user_choice == 2)
        {
            add_find_pdu.type = 'S';
            printf("Enter your requested content name: \n");
            scanf("%s", add_find_pdu.contentname);
            printf("Enter your requested peername: \n");
            scanf("%s", add_find_pdu.peername);

            write(s, &add_find_pdu, 510);

            /* Get response from index server */
            n = read(s, &response, sizeof(response));
            if (n < 0)
            {
                fprintf(stderr, "Read failed\n");
            }

            if (response.type == 'S')
            {
                printf("Index Server found your requested content!\n");
                printf("Address for Content: %s\n", response.data);

                /* Now setup TCP connection between content client and content server*/
                char str[128];
                char *ptr;
                strcpy(str, response.data);
                strtok_r(str, ":", &ptr);
                printf("'%s'  '%s'\n", str, ptr);

                /* The actual setup */
                int sd;
                struct sockaddr_in server;
                struct hostent *hp;
                /* Create a stream socket   */
                if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
                {
                    fprintf(stderr, "Can't creat a socket\n");
                    exit(1);
                }

                bzero((char *)&server, sizeof(struct sockaddr_in));
                server.sin_family = AF_INET;
                server.sin_port = htons(atoi(ptr));
                if (hp = gethostbyname(str))
                    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
                else if (inet_aton(host, (struct in_addr *)&server.sin_addr))
                {
                    fprintf(stderr, "Can't get server's address\n");
                    exit(1);
                }

                /* Connecting to the server */
                if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
                {
                    fprintf(stderr, "Can't connect \n");
                    exit(1);
                }

                char cbuf[255];
                printf("Connected to content server!\n");

                dpdu.type = 'D';
                strcpy(dpdu.contentname, add_find_pdu.contentname);
                size_t contentNameLength = strlen(add_find_pdu.contentname);

                printf("File name sent to content server: %s\n", add_find_pdu.contentname);
                size_t bytes_written = write(sd, &dpdu, 255);

                if (bytes_written < 0)
                {
                    perror("Error writing to socket");
                    // Handle the error, e.g., return or exit
                }
                printf("Bytes written from client: %d\n", bytes_written);
                printf("Preparing to send you content...\n");

                char *trimmed = trimwhitespace(add_find_pdu.contentname);
                char rbuf[255];

                FILE *fp = fopen(trimmed, "w");

                if (fp == NULL)
                {
                    perror("Error opening file for writing.");
                    close(sd);
                    exit(1);
                }

                while ((n = read(sd, &cpdu_res, 255)) > 0)
                {
                    if (cpdu_res.type == 'C')
                    {
                        printf("Before client write to file\n");
                        printf("Received data from server: %.*s\n", n, cpdu_res.data);
                        /*fwrite(cpdu_res.data, 1, n, fp);*/
                        fprintf(fp, cpdu_res.data);
                        printf("After client write to file\n");
                    }
                }

                fclose(fp);
                printf("File downloaded! \n");
                close(sd);

                char server_ip[INET_ADDRSTRLEN];

                printf("Trying to register content to server now... \n");

                /* Setup new content download TCP connection */
                int stcp;
                struct sockaddr_in reg_addr;
                int alen;
                stcp = socket(AF_INET, SOCK_STREAM, 0);
                reg_addr.sin_family = AF_INET;
                reg_addr.sin_port = htons(0);
                reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                /*bind(stcp, (struct sockaddr *)&reg_addr, sizeof(reg_addr));*/
                if (bind(stcp, (struct sockaddr *)&reg_addr, sizeof(reg_addr)) < 0)
                {
                    perror("bind");
                    exit(EXIT_FAILURE);
                }

                alen = sizeof(struct sockaddr_in);
                int check = getsockname(stcp, (struct sockaddr *)&reg_addr, &alen);

                char ip_address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(reg_addr.sin_addr), ip_address, INET_ADDRSTRLEN);
                printf("Address Port: %d\n", ntohs(reg_addr.sin_port));
                printf("Address IP Address: %s\n", ip_address);

                char full_address[255];

                printf("Preparing to register new content to index server... \n");
                printf("What is your peername: \n");
                scanf("%s", client_reg_pdu.peername);
                printf("What is your server address: \n");
                scanf("%s", server_ip);
                client_reg_pdu.type = 'R';
                strcpy(client_reg_pdu.contentname, add_find_pdu.contentname);
                snprintf(full_address, sizeof(full_address), "%s:%d", server_ip, ntohs(reg_addr.sin_port));

                printf("Full Address here is %s\n", full_address);
                strcpy(client_reg_pdu.address, full_address);

                write(s, &client_reg_pdu, 765);
                printf("After new content registration request \n");

                /* Get response from index server */
                n = read(s, &response, sizeof(response));
                if (n < 0)
                {
                    fprintf(stderr, "Read failed\n");
                }

                if (response.type == 'A')
                {
                    printf("Index Server successfully registered your content!\n");
                    if (listen(stcp, 5) < 0)
                    {
                        perror("listen");
                        exit(EXIT_FAILURE);
                    }

                    // Print information about the listening socket
                    printf("Listening on port %d for incoming connections...\n", ntohs(reg_addr.sin_port));
                }

                fd_set rfds, afds;
                int new_sd;
                char buf[255];
                struct sockaddr_in client;

                FD_ZERO(&afds);
                FD_SET(stcp, &afds);
                FD_SET(0, &afds);

                while (1)
                {
                    memcpy(&rfds, &afds, sizeof(rfds));

                    if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0)
                    {
                        perror("select");
                        exit(EXIT_FAILURE);
                    }
                    if (FD_ISSET(0, &rfds))
                    {
                        char userInput[BUFSIZE];
                        fgets(userInput, BUFSIZE, stdin);
                        userInput[strcspn(userInput, "\n")] = '\0'; // Remove newline character

                        if (strcmp(userInput, "q") == 0)
                        {
                            tpdu.type = 'T';

                            strcpy(tpdu.peername, client_reg_pdu.peername);
                            strcpy(tpdu.contentname, trimmed);
                            printf("Peername trying to remove here is %s \n", tpdu.peername);
                            printf("Contentname trying to remove here is %s \n", tpdu.contentname);
                            write(s, &tpdu, 510);

                            n = read(s, &response, sizeof(response));
                            if (n < 0)
                            {
                                fprintf(stderr, "Read failed\n");
                            }

                            if (response.type == 'A')
                            {
                                printf("Closed connection and terminated TCP connection.\n");
                                close(stcp);
                            }
                            else
                            {
                                printf("Error degistering content. \n");
                                close(stcp);
                            }

                            // Handle any other cleanup or termination steps here if needed
                            break; // Exit the loop upon 'q' input
                        }
                    }

                    if (FD_ISSET(stcp, &rfds))
                    {
                        new_sd = accept(stcp, (struct sockaddr *)&client, &alen);
                        if (new_sd != -1)
                        {
                            printf("Got new content client! \n");

                            char sbuf[255];
                            int n;

                            // Read only when the connection is established
                            struct DPDU serverResponse;
                            n = read(new_sd, &serverResponse, 255);
                            printf("serverResponse.type here is %s\n", &serverResponse.contentname);

                            if (serverResponse.type == 'D')
                            {
                                printf("got in d if block\n");
                                cpdu.type = 'C';
                                char *trimmed;
                                trimmed = trimwhitespace(serverResponse.contentname);

                                FILE *fp = fopen(trimmed, "r");
                                if (fp == NULL)
                                {
                                    printf("-> Client requested file that does not exist \n");
                                    write(stcp, "NO FILE", n);
                                    close(stcp);
                                    return 1;
                                }
                                else
                                {
                                    size_t bytesRead;
                                    while ((n = fread(sbuf, 1, 255, fp)) > 0)
                                    {
                                        printf("inside actual while loop");
                                        strcpy(cpdu.data, sbuf);
                                        write(new_sd, &cpdu, 255);
                                        printf("AFTER inside actual while loop");
                                    }
                                    fclose(fp);
                                    close(stcp);
                                }
                                return 0;
                            }
                        }
                    }
                }

                close(sd);
                return 0;
            }
        }
        //    User wants to view content listing
        else if (user_choice == 3)
        {
            opdu.type = 'O';
            write(s, &opdu, 255);
            printf("REGISTERED CONTENT W/ SERVERS\n");
            printf("--------------------------------------------------\n");

            while ((n = read(s, &response, 255)) > 0)
            {
                if (response.type == 'O')
                {
                    printf("%s\n", response.data);
                }
            }
        }
        else if (response.type = 'E')
        {
            printf("Index Server could not find your requested content.\n");
        }
    }
}
