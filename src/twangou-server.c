/*
 ============================================================================
 Name        : twangou-server.c
 Author      : Mark Gao
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sqlite3.h>

#define PORT 8000
#define ISspace(x) isspace((int)(x))
#define MAX_PARTS 10

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void error_die(const char *);
//void execute_cgi(int, const char *, const char *, const char *);
//int get_line(int, char *, int);
//void headers(int, const char *);
//void not_found(int);
//void serve_file(int, const char *);
int startup(u_short *);

void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}

int startup(u_short *port)
{

    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = inet_addr("192.168.1.240");
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    if (*port == 0) /* if dynamically allocating a port */
    {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    if (listen(httpd, 5) < 0)
        error_die("listen");
    return (httpd);
}

static int callback(void *buffer, int argc, char **argv, char **azColName)
{
    int i;
    char *dataPulled = (char *)buffer;

    for (i = 0; i < argc; i++)
    {
        // Append data to the buffer (adjust the concatenation logic as needed)
        //strcat(dataPulled, azColName[i]);
        //strcat(dataPulled, " = ");

        //making sure the query format is first|second|last
        strcat(dataPulled, argv[i] ? argv[i] : "NULL");
        strcat(dataPulled, "|");
        //strcat(dataPulled, "\n");
    }
    /*for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");*/
    return 0;
}

void parseMessage(char *message, char *delimiter, char *parts[], int maxParts)
{

    //seperates string into tokens through the delimeter and points to the first token
    char *input = strdup(message);
    char *token = strtok(input, delimiter);
    int i = 0;

    //runs through the number of tokens and assigns the array each token value
    while (token != NULL && i < maxParts)
    {
        parts[i] = strdup(token); // Use strdup to allocate memory for each part
        //printf("Data Pulled:\n%s\n", parts[i]);

        if (parts[i] == NULL)
        {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }

        ++i;

        //moves to the next token
        token = strtok(NULL, delimiter);
    }
}

//check if a username is available
int checkAvailability(char *columnName, char *tableName, char *valueToCheck, sqlite3 *db)
{
    char *query;
    char *zErrMsg = 0;
    char dataPulled[2048] = {0};

    //prepare query string and execute sqlite3 command
    asprintf(&query, "SELECT %s FROM %s where %s = \"%s\";", columnName, tableName, columnName, valueToCheck);
    sqlite3_exec(db, query, callback, dataPulled, &zErrMsg);
    free(query);

    if (strlen(dataPulled) == 0)
    {
        // available
        return 1;
    }
    else
    {
        //not available
        return 0;
    }
}

int checkCredentials(char *tableName, char *column1, char *column2, char *value1, char *value2, sqlite3 *db)
{
    char *query;
    char *zErrMsg = 0;
    char dataPulled[2048] = {0};

    //prepare query string and execute sqlite3 command
    asprintf(&query, "SELECT * FROM %s where %s = \"%s\" AND %s = \"%s\";", tableName, column1, value1, column2, value2);
    sqlite3_exec(db, query, callback, dataPulled, &zErrMsg);
    free(query);

    if (strlen(dataPulled) == 0)
    {
        // not correct
        return 0;
    }
    else
    {
        //correct
        return 1;
    }
}

void addNewUser(char *userName, char *password, sqlite3 *db)
{
    char *insertCommand;

    asprintf(&insertCommand, "INSERT INTO USERS (USERNAME, PASSWORD) VALUES('%s', '%s');", userName, password);
    sqlite3_exec(db, insertCommand, callback, 0, 0);
}

int main(void)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char *createTable;
    char *insertValues;
    char *pullData;
    char dataPulled[2048] = {0};
    //Open database
    rc = sqlite3_open("twangou.db", &db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    }
    else
    {
        fprintf(stdout, "Opened database successfully\n");
    }

    createTable = "CREATE TABLE IF NOT EXISTS USERS(ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL , USERNAME TEXT NOT NULL, PASSWORD TEXT NOT NULL);";
    // Execute SQL statement
    rc = sqlite3_exec(db, createTable, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Table created successfully\n");
    }

    /*char *deleteValues = "DELETE FROM USERS;";
    rc = sqlite3_exec(db, deleteValues,  callback, 0, &zErrMsg);*/
    /*insertValues = "INSERT INTO USERS (USERNAME, PASSWORD) VALUES('markgao47', 'high4217');";

    rc = sqlite3_exec(db, insertValues,  callback, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Records entered successfully\n");
    }

    pullData = "select * from users;";
    rc = sqlite3_exec(db, pullData, callback, dataPulled, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }
    printf("Data Pulled:\n%s\n", dataPulled);*/
    //printf(dataPulled);

    //sqlite3_close(db);
    int server_sock = -1;
    u_short port = 8080;
    int client_sock = -1;
    char server_message[2000];
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);
    //char *client_message = NULL;
    //initialize input array for server and set each part to null
    char *client_message_parts[MAX_PARTS];
    char delimiter = '|';
    size_t buffer_size = 2000;
    char *client_message = malloc(buffer_size);

    if (client_message == NULL)
    {
        perror("Memory allocation error");
        return -1;
    }

    for (int i = 0; i < MAX_PARTS; ++i)
    {
        client_message_parts[i] = NULL;
    }

    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    while (1 > 0)
    {
        //accept client connection request

        client_sock = accept(server_sock,
                             (struct sockaddr *)&client_name,
                             &client_name_len);
        if (client_sock == -1)
            error_die("accept");

        //receive client message
        ssize_t recv_size;
        if ((recv_size = recv(client_sock, client_message, 2000, 0)) < 0)
        {
            printf("Couldn't receive\n");
            return -1;
        }
        client_message[recv_size] = '\0';

        //parse the client message into an array of different parts
        parseMessage(client_message, &delimiter, client_message_parts, MAX_PARTS);

        //first part will always be the protocol/objective
        char objective[50];
        strcpy(objective, client_message_parts[0]);
        if (strcmp(objective, "SignUp") == 0)
        {
            char username[50];
            strcpy(username, client_message_parts[1]);
            char password[50];
            strcpy(password, client_message_parts[2]);

            int userNameAvailability = checkAvailability("username", "users", username, db);

            if (userNameAvailability == 1)
            {
                addNewUser(username, password, db);
                strcpy(server_message, "Available");
            }
            else
            {
                strcpy(server_message, "Unavailable");
            }
        } else if (strcmp(objective, "SignIn") == 0)
        {
            char username[50];
            strcpy(username, client_message_parts[1]);
            char password[50];
            strcpy(password, client_message_parts[2]);

            int validCredentials = checkCredentials("users", "username", "password", username, password, db);

            if (validCredentials == 1)
            {
                strcpy(server_message, "Correct");
            }
            else
            {
                strcpy(server_message, "Incorrect");
            }
        }
        


        //send message to client
        if (send(client_sock, server_message, strlen(server_message), 0) < 0)
        {
            printf("Can't send\n");
            return -1;
        }
    }
    close(client_sock);
    /* accept_request(client_sock); */
    /*if (pthread_create(&newthread , NULL, accept_request, client_sock) != 0)
   perror("pthread_create");
 }*/
    close(server_sock);
    printf("done");
    return (0);

    /*int httpd = 0
 struct sockaddr_in name;

 httpd = socket(PF_INET, SOCK_STREAM, 0);
 if (httpd == -1)
  error_die("socket");
 memset(&name, 0, sizeof(name));
 name.sin_family = AF_INET;
 name.sin_port = htons(*port);
 name.sin_addr.s_addr = htonl(INADDR_ANY);
 if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
  error_die("bind");
 if (*port == 0)   if dynamically allocating a port 
 {
  int namelen = sizeof(name);
  if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
   error_die("getsockname");
  *port = ntohs(name.sin_port);
 }
 if (listen(httpd, 5) < 0)
  error_die("listen");
 return(httpd);*/

    /*int socket_desc, client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[2000], client_message[2000];
    
    
    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.240");
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");
    
    // Listen for clients:
    if(listen(socket_desc, 5) < 0){
        printf("Error while listening\n");
        return -1;
    }
    printf("\nListening for incoming connections.....\n");
    
    // Accept an incoming connection:
    client_size = sizeof(client_addr);
    client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
    
    if (client_sock < 0){
        printf("Can't accept\n");
        return -1;
    }
    printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    // Receive client's message:
    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
        printf("Couldn't receive\n");
        return -1;
    }
    printf("Msg from client: %s\n", client_message);
    
    // Respond to client:
    strcpy(server_message, "This is the server's message.");
    
    if (send(client_sock, server_message, strlen(server_message), 0) < 0){
        printf("Can't send\n");
        return -1;
    }
    
    // Closing the socket:
    close(client_sock);
    close(socket_desc);
    
    return 0;*/
}
