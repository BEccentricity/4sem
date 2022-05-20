#include <sys/wait.h>
#include "DistributionNetwork.h"

enum {READ = 0, WRITE = 1, ERROR = -1, SUCCESS = 1};

struct ComputerInfo_t {
    size_t numCores;
    size_t numCPU;
};

struct Connection_t {
    int socket;
    struct ComputerInfo_t computerInfo;
};

struct CalculateInfo_t {
    struct Integral_t integral;
    size_t numUsedThreads;
};

//Select где сервер ждет ответа от пользователей, keeplaive прочитать что это такое

//-------------------------------------------Static Functions---------------------------------------
static int FindAndConnectComputers_(struct Connection_t* connections, size_t numComputers);
static double DistributionCalculation_(struct Integral_t integral, size_t numComputers, size_t numThreads,
                                       struct Connection_t *connection);
static int OpenBroadCastSocket_();
static bool SendBroadCast_();
static int ListenBroadCast_(struct sockaddr_in* serverAddr);
int ConnectionsCompare_(struct Connection_t* conn1, struct Connection_t* conn2);
size_t RoundDouble_(double number);
static bool SetKeepalive_(int socket, int keepcnt, int keepidle, int keepintvl);
static int SetServerSocketOptions_();
//------------------------------------------------Debug Functions------------------------------------------
static void PrintConnections_(struct Connection_t* connection, size_t numConnections);
static void PrintCalculateInfo_(struct CalculateInfo_t calculateInfo);


double func(double x) {
    return x;
}

//-------------------------------------------API----------------------------------------------------

DistributionError StartMainNode(size_t numThreads, size_t numComputers, struct Integral_t integral, double* res) {
    DistributionError error = DERROR_OK;
    int serverSocket = ERROR;
    struct Connection_t *connections = NULL;

    if (numComputers == 0 || numThreads == 0) {
        perror("null argument");
        error = DERROR_NULL_ARGUMENT;
        goto exit_handler;
    }
    if (numComputers > numThreads) {
        fprintf(stderr, "\n\t\t\tNUM_THREADS < NUM_COMPUTER, CALCULATION ARE PERFORMED ON ONE COMPUTER\n");
        if (IntegralCalculateWithoutCoresInfo(integral, numThreads, res) != INTEGRAL_SUCCESS) {
            fprintf(stderr, "Integral Calculate ERROR\n");
            error = DERROR_CALCULATION;
        }
        goto exit_handler;
    }

    if (numComputers != 1) {
        connections = calloc(numComputers - 1, sizeof(struct Connection_t));
        serverSocket = FindAndConnectComputers_(connections, numComputers - 1);
        if (serverSocket < 0) {
            perror("find ERROR");
            error = DERROR_FIND;
            goto exit_handler;
        }
    }

    *res = DistributionCalculation_(integral, numComputers, numThreads, connections);
    if (isnan(*res)) {
        perror("distribution ERROR");
        error = DERROR_CALCULATION;
        goto exit_handler;
    }

exit_handler:
    if (connections != NULL) {
        for (int connect = 0; connect < numComputers - 1; ++connect) {
            close(connections[connect].socket);
        }
        close(serverSocket);
        free(connections);
    }

    return error;
}

DistributionError StartSideNode() {
    DistributionError error = DERROR_OK;
    struct sockaddr_in serverAddr;
    struct CoreInfo_t* coresInfo;

    if (ListenBroadCast_(&serverAddr) < 0) {
        return DERROR_CONNECTION;
    }

    fprintf(stderr, "found server IP is %s, Port is %d\n", inet_ntoa(serverAddr.sin_addr),
            htons(serverAddr.sin_port));

    int socketServer = socket(AF_INET, SOCK_STREAM, 0);

    if (!SetKeepalive_(socketServer, 2, 1, 1)) {
        error = DERROR_KEEPALIVE;
        goto exit_handler;
    }

    socklen_t socklen = sizeof(struct sockaddr_in);

    if (connect(socketServer, (const struct sockaddr*) &serverAddr, socklen) < 0) {
        perror("connect");
        error = DERROR_CONNECTION;
        goto exit_handler;
    }

    struct ComputerInfo_t computerInfo = {};
    coresInfo = GetCoresInfo(&computerInfo);

    fprintf(stderr, "Computer info: numCores - %zd, numCPU - %zd\n", computerInfo.numCores, computerInfo.numCPU);

    if (send(socketServer, &computerInfo, sizeof(computerInfo), 0) < 0) {
        perror("send");
        error = DERROR_CONNECTION;
        goto exit_handler;
    }

    struct CalculateInfo_t calculateInfo = {};
    fprintf(stderr, "Client:: Recv tcp msg\n");
    if (recv(socketServer, &calculateInfo, sizeof(calculateInfo), 0) <= 0) {
        fprintf(stderr, "Recv ERROR");
        error = DERROR_CONNECTION;
        goto exit_handler;
    }

    //fprintf(stderr, "{%f -> %f}", calculateInfo.integral.begin, calculateInfo.integral.end);
    fprintf(stderr, "Client:: Recved tcp msg\n");

    PrintCalculateInfo_(calculateInfo);

    double res = 0.0;
    calculateInfo.integral.func = func;
    if (IntegralCalculate(coresInfo, &computerInfo, calculateInfo.integral, calculateInfo.numUsedThreads, &res) != INTEGRAL_SUCCESS) {
        fprintf(stderr, "Integral Calculate ERROR\n");
        error = DERROR_CALCULATION;
        goto exit_handler;
    }

    if (send(socketServer, &res, sizeof(res), 0) < 0) {
        perror("send");
        error = DERROR_CONNECTION;
        goto exit_handler;
    }

    int okMsg = 0;
    if (recv(socketServer, &okMsg, sizeof(res), 0) <= 0) {
        fprintf(stderr, "Recv ERROR\n");
        error = DERROR_CONNECTION;
        goto exit_handler;
    }

exit_handler:
    close(socketServer);
    PrintCoresInfo(coresInfo, computerInfo.numCores);
    if (coresInfo != NULL) {
        FreeCoresInfo(coresInfo, computerInfo.numCores);
    }
    return error;
}

//-------------------------------------------Static Functions---------------------------------------
static int FindAndConnectComputers_(struct Connection_t* connections, size_t numComputers) {
    assert(connections);

    int socketServer = SetServerSocketOptions_();
    if (socketServer < 0) {
        return -1;
    }

    struct sockaddr_in serverAddr = {.sin_family = AF_INET,
                                     .sin_port = htons(TCP_PORT),
                                     .sin_addr.s_addr = htonl(INADDR_ANY)};
    socklen_t socklen = sizeof(struct sockaddr_in);

    if(bind(socketServer, (const struct sockaddr *) &serverAddr, socklen) < 0) {
        perror("bind");
        return -1;
    }

    if (!SendBroadCast_()) {
        return -1;
    }
    PrintConnections_(connections, numComputers - 1);

    if(listen(socketServer, numComputers + 1) < 0) {
        perror("listen");
        return -1;
    }

    for (int itSockets = 0 ; itSockets < numComputers; ++itSockets) {
        int clientSock = accept(socketServer, (struct sockaddr *) &serverAddr, &socklen);
        fprintf(stderr, "User connect by TCP with IP: %s\n", inet_ntoa(serverAddr.sin_addr));
        if (clientSock < 0) {
            perror ("accept");
            return -1;
        }
        connections[itSockets].socket = clientSock;
        if (recv(clientSock, &connections[itSockets].computerInfo, sizeof(struct ComputerInfo_t), 0) <= 0) {
            fprintf(stderr, "Recv ERROR");
            return -1;
        }
        fprintf(stderr, "Get computer info\n");
    }
    return socketServer;
}

static double DistributionCalculation_(struct Integral_t integral, size_t numComputers, size_t numThreads,
                                       struct Connection_t *connection) {
    assert(connection);
    assert(numThreads > 0);
    assert(numComputers > 0);

    double result = NAN;
    struct CalculateInfo_t* calculateInfos = NULL;
    struct CoreInfo_t* coresInfo = NULL;

    if (numComputers == 1) {
        double res = 0.0;
        if (IntegralCalculateWithoutCoresInfo(integral, numThreads, &res) != INTEGRAL_SUCCESS) {
            fprintf(stderr, "Integral Calculate ERROR\n");
            result = NAN;
            goto exit_handler;
        }
        return res;
    }
    assert(connection);
    calculateInfos = calloc(numComputers, sizeof(struct CalculateInfo_t));
    double dataStep = (integral.end  - integral.begin) / numComputers;
    struct ComputerInfo_t thisComputerInfo = {};
    coresInfo = GetCoresInfo(&thisComputerInfo);

    size_t numAllCPU = thisComputerInfo.numCPU;
    for ( size_t itComputer = 0; itComputer < numComputers - 1; ++itComputer) {
        numAllCPU += connection[itComputer].computerInfo.numCPU;
        fprintf(stderr, "Client %zd: numCPU - %zd\n", itComputer, connection[itComputer].computerInfo.numCPU);
    }
    assert(numThreads - numComputers >= 0);
//    size_t distributionThreads = numThreads - numComputers;
//
//    for (size_t itComputer = 0; itComputer < numComputers - 1; ++itComputer) {
//        calculateInfos[itComputer].integral.begin = integral.begin + itComputer * dataStep;
//        calculateInfos[itComputer].integral.end = integral.begin + (itComputer + 1) * dataStep;
//        calculateInfos[itComputer].integral.func = integral.func;
//        size_t usedThread = 1 + RoundDouble_((((double)connection[itComputer].computerInfo.numCPU / (double)numAllCPU) *
//                                           (double)distributionThreads));
//        if (numThreads >= usedThread) {
//            calculateInfos[itComputer].numUsedThreads = usedThread;
//            numThreads -= usedThread;
//        } else {
//            calculateInfos[itComputer].numUsedThreads = numThreads;
//            numThreads = 0;
//            fprintf(stderr, "Something strange is happening");
//        }
//    }
//
//    calculateInfos[numComputers - 1].integral.begin = integral.begin + (numComputers - 1) * dataStep;
//    calculateInfos[numComputers - 1].integral.end = integral.begin + numComputers * dataStep;
//    calculateInfos[numComputers - 1].integral.func = integral.func;
//    calculateInfos[numComputers - 1].numUsedThreads = numThreads;


    for (size_t itComputer = 0; itComputer < numComputers; ++itComputer) {
        calculateInfos[itComputer].integral.begin = integral.begin + itComputer * dataStep;
        calculateInfos[itComputer].integral.end = integral.begin + (itComputer + 1) * dataStep;
        calculateInfos[itComputer].integral.func = integral.func;
    }

    while(numThreads != 0) {
        for (size_t itComputer = 0; itComputer < numComputers; ++itComputer) {
            calculateInfos[itComputer].numUsedThreads++;
            numThreads--;
            if(numThreads == 0) {
                break;
            }
        }
    }


    double res = 0.0;

    PrintCalculateInfo_(calculateInfos[numComputers - 1]);

        for (size_t itConnect = 0; itConnect < numComputers - 1; ++itConnect) {
            if (send(connection[itConnect].socket,
                     &calculateInfos[itConnect], sizeof(calculateInfos[itConnect]), 0) < 0) {
                perror("send");
                result = NAN;
                goto exit_handler;
            }
        }
        if (IntegralCalculate(coresInfo, &thisComputerInfo, calculateInfos[numComputers - 1].integral,
                          calculateInfos[numComputers - 1].numUsedThreads, &res) != INTEGRAL_SUCCESS) {
            fprintf(stderr, "Integral Calculate ERROR\n");
            result = NAN;
            goto exit_handler;
        }

        for (size_t itConnect = 0; itConnect < numComputers - 1; ++itConnect) {
            fd_set rfds;
            struct timeval tv = {TIMEOUT, 0};
            FD_ZERO(&rfds);
            FD_SET(connection[itConnect].socket, &rfds);
            int retval;
            retval = select(connection[itConnect].socket + 1, &rfds,
                            NULL, NULL, &tv);
            if (retval <= 0) {
                exit(ERROR);
            }

            double connectRes = 0;
            if (recv(connection[itConnect].socket, &connectRes, sizeof(connectRes), 0) <= 0) {
                fprintf(stderr, "Recv ERROR\n");
                res = NAN;
                goto exit_handler;
            }
            int okMsg = 1;

            if (send(connection[itConnect].socket, &okMsg, sizeof okMsg, 0) < 0) {
                perror("send");
                res = NAN;
                goto exit_handler;
            }
            res += connectRes;
        }


exit_handler:
        if (calculateInfos != NULL) {
            free(calculateInfos);
        }
        if (coresInfo != NULL) {
            FreeCoresInfo(coresInfo, thisComputerInfo.numCores);
        }

        return res;

}

static bool SendBroadCast_() {
    int socketBC = OpenBroadCastSocket_();
    if (socketBC < 0) {
        return false;
    }

    struct sockaddr_in broadcastAddr = {.sin_family = AF_INET,
                                        .sin_addr.s_addr = htonl(INADDR_BROADCAST),
                                        .sin_port = htons(BROADCAST_PORT)};

    if (sendto(socketBC, "IP_FOUND", strlen("IP_FOUND"), 0, (struct sockaddr*) &broadcastAddr, sizeof (broadcastAddr)) < 0) {
        perror("sendto");
        return false;
    }
    close(socketBC);
    return true;
}

static int ListenBroadCast_(struct sockaddr_in* serverAddr) {
    int socketRc = socket(AF_INET, SOCK_DGRAM, 0);

    char buffer[SIZE_BUFF] = {};
    struct sockaddr_in socketAddr = {.sin_family = AF_INET,
                                     .sin_port = htons(BROADCAST_PORT),
                                     .sin_addr.s_addr = htonl(INADDR_ANY)};

    socklen_t addrLen = sizeof(struct sockaddr_in);

    if (bind(socketRc, (struct sockaddr*) &socketAddr, addrLen) < 0) {
        perror("bind");
        return -1;
    }
    if (recvfrom(socketRc, buffer, SIZE_BUFF, 0, (struct sockaddr *) serverAddr, &addrLen) < 0) {
        perror("recvfrom");
        return -1;
    }
    serverAddr->sin_port = htons(TCP_PORT);
    fprintf(stderr, "found server IP is %s, Port is %d\n", inet_ntoa(serverAddr->sin_addr),
            htons(serverAddr->sin_port));
    return 1;
}

static int OpenBroadCastSocket_() {
    int socketBC = socket(PF_INET, SOCK_DGRAM, 0);
    if (socketBC < 0) {
        perror("socket");
        return -1;
    }

    int options = 1;
    if(setsockopt(socketBC, SOL_SOCKET, SO_BROADCAST, &options, sizeof(options)) < 0) {
        perror("sotsockopt");
        return -1;
    }

    return socketBC;
}

void GetCurrentIpAddr(char* buff) {
    struct ifaddrs *addrs;
    getifaddrs(&addrs);
    struct ifaddrs *tmp = addrs;

    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && strcmp(tmp->ifa_name, "lo") != 0)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            sprintf( buff, "%s", inet_ntoa(pAddr->sin_addr));
        }

        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
}

int ConnectionsCompare_(struct Connection_t* conn1, struct Connection_t* conn2) {
    if (conn1->computerInfo.numCPU < conn2->computerInfo.numCPU) {
        return -1;
    }
    if (conn1->computerInfo.numCPU > conn2->computerInfo.numCPU) {
        return 1;
    }
    return 0;
}

size_t RoundDouble_(double number) {
    size_t intNum = (size_t) number;
    if (number - intNum != 0) {
        intNum++;
    }
    return intNum;
}

static bool SetKeepalive_(int socket, int keepcnt, int keepidle, int keepintvl) {
    int ret;
    int optTrue = 1;

    ret = setsockopt (socket, SOL_SOCKET, SO_KEEPALIVE, &optTrue, sizeof(int));
    if (ret != 0) {
        perror ("setsockopt");
        return false;
    }

    ret = setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
    if (ret != 0) {
        perror ("tcp_keepcnt");
        return false;
    }

    ret = setsockopt (socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
    if (ret != 0) {
        perror ("tcp_keepcnt");
        return false;
    }

    ret = setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
    if (ret != 0) {
        perror ("tcp_keepcnt");
        return false;
    }

    return true;
}

static int SetServerSocketOptions_() {
    int socketServer = socket(AF_INET, SOCK_STREAM, 0);
    int optTrue = 1;

    if (setsockopt (socketServer, SOL_SOCKET, SO_REUSEADDR, &optTrue, sizeof(int)) != 0) {
        perror ("setsockopt for server socket:");
        return -1;
    }

    struct timeval timeout = {
            .tv_sec = 10,
            .tv_usec = 0
    };

    if (setsockopt (socketServer, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) != 0) {
        perror ("setsockopt (timeout) for server socket:");
        return -1;
    }

    if (!SetKeepalive_(socketServer, 4, 3, 1)) {
        return -1;
    }

    return socketServer;
}

//------------------------------------------------Debug Functions------------------------------------------
static void PrintConnections_(struct Connection_t* connection, size_t numConnections) {
    assert(connection);
    for (int i = 0; i < numConnections; ++i) {
        printf("Comp%d: numCores - %zu, numCPUByCore - %zu\n",
               i, connection[i].computerInfo.numCores, connection[i].computerInfo.numCores);
    }
}

static void PrintCalculateInfo_(struct CalculateInfo_t calculateInfo) {
    fprintf(stderr, "---------------------------Calculate Info--------------------------------------\n");
    fprintf(stderr, "\t\t\t\t\tNum used threads: %zu\n", calculateInfo.numUsedThreads);
    fprintf(stderr, "\t\t\t\t\tIntegral: %f to %f\n", calculateInfo.integral.begin, calculateInfo.integral.end);
    fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
}
