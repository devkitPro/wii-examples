#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>

#include <debug.h>
#include <errno.h>
#include <wiiuse/wpad.h>

#include <ogc/if_config.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

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
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}


static s32 create_socket(u16 port) {
    int sock;
    int ret;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sock < 0) {
        perror ("socket");
        return -1;
    }

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    ret = bind(sock, (struct sockaddr *)&server, sizeof(server));
    if (ret) {
        perror("bind");
        return -1;
    }

    socklen_t len = sizeof(server);
    ret = getsockname(sock, (struct sockaddr *)&server, &len);
    if (ret < 0) {
        perror("getsockname");
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
    /* Note: passing MSG_DONTWAIT in the flags makes recvfrom() return
     * error EOPNOTSUPP */
    len = recvfrom(sock, buffer, sizeof(buffer), 0,
                       (struct sockaddr *)&client, &sock_len);
    if (len < 0) {
        printf("recvfrom returned error %d\n", len);
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
    sendto(sock, buffer_out, len_out, 0, (struct sockaddr *)&client, sock_len);
    if (strncmp(buffer, "exit", 4) == 0) exit(0);
}

int main(int argc, char **argv) {
    s32 ret;

    bool network_up = false;

    char localip[16] = {0};
    char gateway[16] = {0};
    char netmask[16] = {0};

    struct pollfd fds[NUM_SOCKETS];

    initialise();

    printf("\nlibogc network UDP demo\n");
    printf("Configuring network...\n");

    // Configure the network interface
    ret = if_config ( localip, netmask, gateway, true, 20);
    if (ret>=0) {
        printf ("network configured, ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);

        for (int i = 0; i < NUM_SOCKETS; i++) {
            s32 sock = create_socket(45678 + i);
            if (sock < 0) return EXIT_FAILURE;
            fds[i].fd = sock;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
        }

        network_up = true;
        printf("\nTo test this program, run this command from a terminal:\n"
               "\n   nc -u %s 45678 (or 45679, ...)\n"
               "\nthen type something and press ENTER.\n\n", localip);


    } else {
        printf ("network configuration failed!\n");
    }

    while (1) {
        VIDEO_WaitVSync();
        WPAD_ScanPads();

        int buttonsDown = WPAD_ButtonsDown(0);

        if (buttonsDown & WPAD_BUTTON_1) {
            exit(0);
        }

        /* This is not really needed in this case, but let's show how polling
         * works */
        if (network_up) {
            s32 timeout = 10;
            ret = poll(fds, NUM_SOCKETS, timeout);
            if (ret > 0) {
                for (int i = 0; i < NUM_SOCKETS; i++) {
                    if (fds[i].revents) {
                        handle_client(fds[i].fd);
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
