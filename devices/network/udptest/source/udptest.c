#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <wiiuse/wpad.h>

#define NUM_SOCKETS 4

static void initialise() {
    GXRModeObj *rmode = NULL;
    void *framebuffer;

    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(framebuffer, 20, 20, rmode->fbWidth, rmode->xfbHeight,
                 rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(framebuffer);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_ClearFrameBuffer(rmode,xfb,COLOR_BLACK);
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}

static void show_help()
{
    char local_ip[16] = {0};
    const char *own_ip;
    s32 ret;

    /* Note: this call is not needed to initialize the network. We just need it
     * to be able to show our current IP address in the message below. */
    ret = if_config(local_ip, NULL, NULL, true, 20);
    if (ret < 0) {
        printf("Error retrieving own IP address.\n");
        own_ip = "<console-ip-address>";
    } else {
        own_ip = local_ip;
    }

    printf("\nTo test this program, run this command from a terminal:\n"
           "\n   nc -u %s 45678 (or 45679, ...)\n"
           "\nthen type something and press ENTER.\n\n", own_ip);
}

static s32 create_socket(u16 port) {
    int sock;
    int ret;
    struct sockaddr_in server;

    sock = net_socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sock == INVALID_SOCKET) {
        printf ("Cannot create a socket!\n");
        return -1;
    }

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    ret = net_bind(sock, (struct sockaddr *)&server, sizeof(server));
    if (ret) {
        printf("Error %d binding socket!\n", ret);
        return -1;
    }

    socklen_t len = sizeof(server);
    ret = net_getsockname(sock, (struct sockaddr *)&server, &len);
    if (ret < 0) {
        printf("Error %d getting socket address!\n", ret);
        return -1;
    }

    u32 addr = server.sin_addr.s_addr;
    printf("UDP socket listening on: %u.%u.%u.%u:%u\n",
           (addr >> 24) & 0xff,
           (addr >> 16) & 0xff,
           (addr >> 8) & 0xff,
           addr & 0xff,
           server.sin_port);
    return sock;
}

static void handle_client(s32 sock)
{
    char buffer[256], buffer_out[256 + 16];
    socklen_t sock_len;
    struct sockaddr_in client;
    s32 len, len_out;

    sock_len = sizeof(client);
    /* Note: passing MSG_DONTWAIT in the flags makes net_recvfrom() return
     * error EOPNOTSUPP */
    len = net_recvfrom(sock, buffer, sizeof(buffer), 0,
                       (struct sockaddr *)&client, &sock_len);
    if (len < 0) {
        printf("net_recvfrom returned error %d\n", len);
        return;
    }
    u32 addr = client.sin_addr.s_addr;
    printf("Received %d bytes from %u.%u.%u.%u:%u: %.*s\n", len,
           (addr >> 24) & 0xff,
           (addr >> 16) & 0xff,
           (addr >> 8) & 0xff,
           addr & 0xff,
           client.sin_port, len, buffer);

    len_out = snprintf(buffer_out, sizeof(buffer_out), "Echo: %.*s\n", len, buffer);
    net_sendto(sock, buffer_out, len_out, 0, (struct sockaddr *)&client, sock_len);
    if (strncmp(buffer, "exit", 4) == 0) exit(0);
}

int main(int argc, char **argv) {
    s32 ret;

    initialise();

    printf("\nlibogc network UDP demo\n");
    printf("Configuring network...\n");

    net_init();

    struct pollsd sds[NUM_SOCKETS];
    for (int i = 0; i < NUM_SOCKETS; i++) {
        s32 sock = create_socket(45678 + i);
        if (sock < 0) return EXIT_FAILURE;

        sds[i].socket = sock;
        sds[i].events = POLLIN;
        sds[i].revents = 0;
    }

    show_help();

    while (1) {
        VIDEO_WaitVSync();
        WPAD_ScanPads();

        int buttonsDown = WPAD_ButtonsDown(0);

        if (buttonsDown & WPAD_BUTTON_1) {
            exit(0);
        }

        /* This is not really needed in this case, but let's show how polling
         * works */
        s32 timeout = 10;
        ret = net_poll(sds, NUM_SOCKETS, timeout);
        if (ret > 0) {
            for (int i = 0; i < NUM_SOCKETS; i++) {
                if (sds[i].revents) {
                    handle_client(sds[i].socket);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
