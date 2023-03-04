#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"


void takevaluesfromjsonstring(char *data, char *key, char *value) //functie ce extrage key si value dintr-un singur obiect json
{
    int aux=0;
    char *p=data+2;

    while(*p!='"')
    {
        key[aux]=*p;
        aux++;
        p++;
    }
    key[aux]='\0';
    p=p+3;
    aux=0;

    while(*p!='"')
    {
        value[aux]=*p;
        aux++;
        p++;
    }
    value[aux]='\0';
}

int main(int argc, char *argv[])
{
    char *message; //requestul ce trebuie trimis catre server
    char *response, command[30]; //raspunsul serverului si comanda data de la tastatura
    int sockfd; //socketul de pe care primim/dam cereri
    int state=0; //starea clientului (0-neconectat, 1-conectat)

    char **cookies = calloc(1, sizeof(char *)); //vectorul de cookies (in cazul nostru o sa avem doar un cookie)
    int totalcookies=0; //nr de cookies total

    char *header_token=NULL; //numele headerului pt tokenul JWT (Authorization)
    char *header_token_data=NULL; //tokenul JWT

    while(1)
    {
        scanf("%s", command); //luam comanda de la tastatura

        if(strcmp(command, "register")==0) //comanda REGISTER
        {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea

            char username[100];
            char password[100];

            printf("username="); //campurile de username si password
            scanf("%s", username);
            printf("password=");
            scanf("%s", password);

            JSON_Value *root_value= json_value_init_object();
            JSON_Object *root_object= json_value_get_object(root_value);

            json_object_set_string(root_object, "username", username);
            json_object_set_string(root_object, "password", password);

            char *string_json=NULL;

            string_json=json_serialize_to_string_pretty(root_value); //am creat obiectul json cu cele 2 campuri

            message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/register", "application/json", &string_json, 1, NULL, 0, NULL, NULL); //post request
            send_to_server(sockfd, message); //trimitem mesajul catre server
            response = receive_from_server(sockfd); //primim raspunsul serverului

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocolul si status_code din raspuns

            if(status_code!=200 && status_code!=201) //caz de eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response)); //luam obiectul json din raspuns (eroarea)
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value); //printam eroarea
                free(data);
            }
            else if(status_code == 201) //inregistrare cu succes
            {
                printf("%d - OK - Register successful!\n", status_code);
            }

            close(sockfd); //inchidem socketul
            json_free_serialized_string(string_json);
            json_value_free(root_value);
        }

        if(strcmp(command, "login")==0) //comanda LOGIN
        {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea

            char username[100];
            char password[100];

            printf("Username="); //campurile de username si password
            scanf("%s", username);
            printf("Password=");
            scanf("%s", password);

            if(state == 1) //caz in care suntem deja logati
            {
                printf("You're already logged in.\n");
                continue;
            }

            JSON_Value *root_value= json_value_init_object();
            JSON_Object *root_object= json_value_get_object(root_value);

            json_object_set_string(root_object, "username", username);
            json_object_set_string(root_object, "password", password);

            char *string_json=NULL;

            string_json=json_serialize_to_string_pretty(root_value); //am creat obiectul json cu campurile respective

            message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/login", "application/json", &string_json, 1, NULL, 0, NULL, NULL); //post request
            send_to_server(sockfd, message); //trimitem requestul
            response = receive_from_server(sockfd); //primim raspunsul

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocol si status_code al raspunsului

            if(status_code!=200) //caz eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response)); //luam obiectul json din raspuns(eroarea)
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value); //afisam eroarea
                free(data);
            }
            else //caz succes in care extragem din header cookie-ul de sesiune
            {
                state=1;
                cookies[totalcookies] = calloc(LINELEN, sizeof(char));
                char *p;
                int aux=0;
                p=strstr(response, "Set-Cookie"); //cautam headerul pentru cookie
                p=p+12;

                while(*p!=';') //copiem cookie-ul in vectorul nostru de cookies
                {
                    cookies[totalcookies][aux]=*p;
                    p++;
                    aux++;
                }

                cookies[totalcookies][aux]='\0';
                totalcookies++;
                printf("%d - OK - Login successful!\n", status_code); //afisam mesaj de succes
            }
            
            close(sockfd);
            json_free_serialized_string(string_json);
            json_value_free(root_value);
        }

        if(strcmp(command, "enter_library")==0) //comanda ENTRY_LIBRARY
        {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea

            if(state == 0) //caz in care nu suntem logati (trimitem NULL la campul de cookies)
            {
                message = compute_get_request("34.241.4.235", "/api/v1/tema/library/access", NULL, NULL, 0, NULL, NULL); //get request
            }
            else
            {
                message = compute_get_request("34.241.4.235", "/api/v1/tema/library/access", NULL, cookies, 1, NULL, NULL); //get request
            }
            
            send_to_server(sockfd, message); //trimitem requestul
            response = receive_from_server(sockfd); //primim raspunsul

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocol si status_code al raspunsului

            if(status_code!=200) //caz eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response)); //extragem eroarea
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value); //afisare eroare
                free(data);
            }
            else //caz succes si extragem tokenul
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response));
                char key[50], value[100];
                
                header_token=calloc(LINELEN, sizeof(char));
                header_token_data=calloc(LINELEN, sizeof(char));
                memset(header_token, 0, LINELEN);
                memset(header_token_data, 0, LINELEN);

                takevaluesfromjsonstring(data, key, value);

                strcpy(header_token, "Authorization"); //headerul
                char auxline[500];
                strcpy(auxline, "Bearer ");
                strcat(auxline, value);
                strcpy(header_token_data, auxline); //tokenul JWT
                free(data);

                printf("%d -OK - Enter_library successful!\n", status_code);
            }
            
            close(sockfd);
        }

        if(strcmp(command, "get_books")==0) //comanda GET_BOOKS
        {
            if(state == 0) //cazul in care nu suntem logati
            {
                printf("You are not logged in.\n");
                continue;
            }

            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea

            message = compute_get_request("34.241.4.235", "/api/v1/tema/library/books", NULL, cookies, 1, header_token, header_token_data); //get request

            send_to_server(sockfd, message); //trimitem requestul
            response = receive_from_server(sockfd); //primim raspunsul

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocol si status_code al raspunsului

            if(status_code!=200) //caz eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response));
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value);
                free(data);
            }
            else //caz succes si avem un array de obiecte json
            {
                char *data = calloc(LINELEN, sizeof(char));
                
                if(basic_extract_json_response(response) == NULL) //nu avem obiecte json in lista de carti
                {
                    printf("[]\n");
                }
                else
                {
                    strcpy(data, basic_extract_json_response(response));

                    int n=strlen(data);
                    data[n-1]='\0';
                    printf("%s\n", data);
                }

                free(data);
            }
            
            close(sockfd);
        }

        if(strcmp(command, "get_book")==0) //comanda GET_BOOK
        {
            int id;

            printf("ID=");
            scanf("%d", &id);

            char url[200], id_string[50];

            sprintf(id_string, "%d", id);
            strcpy(url, "/api/v1/tema/library/books/");
            strcat(url, id_string);

            if(state ==0)
            {
                printf("You are not logged in.\n");
                continue;
            }

            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea
            
            message = compute_get_request("34.241.4.235", url, NULL, cookies, 1, header_token, header_token_data); //get request

            send_to_server(sockfd, message); //trimitem requestul
            response = receive_from_server(sockfd); //primim raspunsul

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocol si status_code al raspunsului

            if(status_code!=200) //caz eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response));
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value);
                free(data);
            }
            else //caz succes si avem un obiect json
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response));

                int n=strlen(data);
                data[n-1]='\0';
                printf("%s\n", data);
                free(data);
            }
            
            close(sockfd);
        }

        if(strcmp(command, "add_book")==0) //comanda ADD_BOOK
        {
            char title[100], author[100], genre[100], publisher[100];
            int page_count, ok=1;
            char muda;

            scanf("%c", &muda);

            printf("Title=");
            fgets(title, 100, stdin);
            if(strlen(title)==1)
            {
                ok=0;
            }
            else
            {
                title[strlen(title)-1]='\0';
            }
            

            printf("Author=");
            fgets(author, 100, stdin);
            if(strlen(author)==1)
            {
                ok=0;
            }
            else
            {
                author[strlen(author)-1]='\0';
            }

            printf("Genre=");
            fgets(genre, 100, stdin);
            if(strlen(genre)==1)
            {
                ok=0;
            }
            else
            {
                genre[strlen(genre)-1]='\0';
            }

            printf("Publisher=");
            fgets(publisher, 100, stdin);
            if(strlen(publisher)==0)
            {
                ok=0;
            }
            else
            {
                publisher[strlen(publisher)-1]='\0';
            }

            printf("Page_count=");
            if(scanf("%d", &page_count)==0)
            {
                ok=0;
            }

            if(ok==0) //caz input invalid
            {
                printf("Input is not valid! Try again...\n");
                continue;
            }

            JSON_Value *root_value= json_value_init_object();
            JSON_Object *root_object= json_value_get_object(root_value);

            json_object_set_string(root_object, "title", title);
            json_object_set_string(root_object, "author", author);
            json_object_set_string(root_object, "genre", genre);
            json_object_set_number(root_object, "page_count", (double)page_count);
            json_object_set_string(root_object, "publisher", publisher);

            char *string_json=NULL;

            string_json=json_serialize_to_string_pretty(root_value); //am creat obiectul json cu campurile respective

            if(state == 0) //caz nu suntem logati
            {
                printf("You are not logged in.\n");
                json_free_serialized_string(string_json);
                json_value_free(root_value);
                continue;
            }

            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea

            message = compute_post_request("34.241.4.235", "/api/v1/tema/library/books", "application/json", &string_json, 1, cookies, 1, header_token, header_token_data); //post request
            
            send_to_server(sockfd, message); //trimitem requestul
            response = receive_from_server(sockfd); //primim raspunsul

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocol si status_code al raspunsului

            if(status_code == 429) //"Too many requests" 
            {
                printf("Slow down there buddy! Too many requests, wait a little bit.\n");
                close(sockfd);
                json_free_serialized_string(string_json);
                json_value_free(root_value);
                continue;

            }
            if(status_code!=200) //caz eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response));
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value);
                free(data);
            }
            else //caz succes
            {
                printf("%d - OK - Carte adaugata cu succes!\n", status_code);
            }
            
            close(sockfd);
            json_free_serialized_string(string_json);
            json_value_free(root_value);

        }
        if(strcmp(command, "delete_book")==0) //comanda DELETE_BOOK
        {
            int id;

            printf("ID=");
            scanf("%d", &id);

            char url[200], id_string[50];

            //formam url-ul
            sprintf(id_string, "%d", id);
            strcpy(url, "/api/v1/tema/library/books/");
            strcat(url, id_string);

            if(state == 0)
            {
                printf("You are not logged in.\n");
                continue;
            }

            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea
            
            message = compute_delete_request("34.241.4.235", url, NULL, cookies, 1, header_token, header_token_data); //delete request

            send_to_server(sockfd, message); //trimitem requestul
            response = receive_from_server(sockfd); //primim raspunsul

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocol si status_code al raspunsului

            if(status_code!=200) //caz eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response));
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value);
                free(data);
            }
            else //caz succes
            {
                printf("%d - OK - Book deleted successfully!\n", status_code);
            }
            
            close(sockfd);
        }

        if(strcmp(command, "logout")==0) //comanda LOGOUT
        {
            sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0); //deschidem conexiunea

            if(totalcookies==0)
            {
                message = compute_get_request("34.241.4.235", "/api/v1/tema/auth/logout", NULL, NULL, 0, NULL, NULL); //get request
            }
            else
            {
                message = compute_get_request("34.241.4.235", "/api/v1/tema/auth/logout", NULL, cookies, 1, NULL, NULL); //get request
            }
            
            send_to_server(sockfd, message); //trimitem requestul
            response = receive_from_server(sockfd); //primim raspunsul

            char protocol[200];
            int status_code;
            memset(&status_code, 0, 4);

            sscanf(response, "%s %d", protocol, &status_code); //protocol si status_code al raspunsului

            if(status_code!=200) //caz eroare
            {
                char *data = calloc(LINELEN, sizeof(char));
                strcpy(data, basic_extract_json_response(response));
                char key[50], value[50];
                
                takevaluesfromjsonstring(data, key, value);
                printf("%s : %s\n", key, value);
                free(data);
            }
            else //caz succes
            {
                printf("%d - OK - Logout successful!\n", status_code);
                state=0; //deconectat
                totalcookies--;
                memset(cookies[totalcookies], 0, LINELEN); //stergem cookie-ul
                free(header_token); //stergem token-ul
                free(header_token_data);
                header_token=NULL;
                header_token_data=NULL;
            }
            
            close(sockfd);
        }
        if(strcmp(command, "exit")==0) //comanda EXIT
        {
            break;
        }
    }

    //eliberari memorie
    if(header_token !=NULL && header_token_data!=NULL)
    {
        free(header_token);
        free(header_token_data);
    }
    for(int i=0; i<totalcookies; i++)
    {
        free(cookies[i]);
    }
    free(cookies);
    return 0;
}
