#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <ogcsys.h>
#include <gccore.h>
#include <ogc/if_config.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void *initialise();
void *httpd (void *arg);

static	lwp_t httd_handle = (lwp_t)NULL;

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	s32 ret;

	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};

	xfb = initialise();

	printf ("\nlibogc network demo\n");
	printf("Configuring network ...\n");

	// Configure the network interface
	ret = if_config ( localip, netmask, gateway, true, 20);
	if (ret>=0) {
		printf ("network configured, ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);

		LWP_CreateThread(	&httd_handle,	/* thread handle */
							httpd,			/* code */
							localip,		/* arg pointer for thread */
							NULL,			/* stack base */
							64*1024,		/* stack size */
							50				/* thread priority */ );
	} else {
		printf ("network configuration failed!\n");
	}

	while(1) {

		VIDEO_WaitVSync();
		WPAD_ScanPads();

		int buttonsDown = WPAD_ButtonsDown(0);

		if (buttonsDown & WPAD_BUTTON_HOME) {
			break;
		}
	}

	return 0;
}

const static char http_200[] = "HTTP/1.1 200 OK\r\n";

const static char indexdata[] = "<html> \
                               <head><title>A test page</title></head> \
                               <body> \
                               This small test page has had %d hits. \
                               </body> \
                               </html>";

const static char http_html_hdr[] = "Content-type: text/html\r\n\r\n";
const static char http_get_index[] = "GET / HTTP/1.1\r\n";

//---------------------------------------------------------------------------------
void *httpd (void *arg) {
//---------------------------------------------------------------------------------

	int sock, csock;
	int ret;
	u32	clientlen;
	struct sockaddr_in client;
	struct sockaddr_in server;
	char temp[1026];
	static int hits=0;

	clientlen = sizeof(client);

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sock < 0) {
		perror("socket");
    } else {

    	printf("socket = %d\n", sock);

		memset (&server, 0, sizeof (server));
		memset (&client, 0, sizeof (client));

		server.sin_family = AF_INET;
		server.sin_port = htons (80);
		server.sin_addr.s_addr = INADDR_ANY;
		ret = bind (sock, (struct sockaddr *) &server, sizeof (server));

		if ( ret ) {
			perror("bind");
		} else {

			ret = listen( sock, 5);
			if (ret) {
				perror("listen");
			} else {

				while(1) {

					csock = accept (sock, (struct sockaddr *) &client, &clientlen);

					if ( csock < 0 ) {
						perror("accept");
						while(1);
					}

					printf("Connecting port %d from %s\n", client.sin_port, inet_ntoa(client.sin_addr));
					memset (temp, 0, 1026);
					ret = recv (csock, temp, 1024, 0);
					printf("Received %d bytes\n", ret);

					if ( !strncmp( temp, http_get_index, strlen(http_get_index) ) ) {
						hits++;
						send(csock, http_200, strlen(http_200), 0);
						send(csock, http_html_hdr, strlen(http_html_hdr), 0);
						sprintf(temp, indexdata, hits);
						send(csock, temp, strlen(temp), 0);
					}

					close (csock);

				}
			}
		}
	}

	close(sock);
	return NULL;
}

//---------------------------------------------------------------------------------
void *initialise() {
//---------------------------------------------------------------------------------

	void *framebuffer;

	VIDEO_Init();
	WPAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(framebuffer,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	return framebuffer;

}
