#include "ping.h"
#include "signal.h"
#include "stdio.h"
#include "sys/time.h"
#include <asm-generic/socket.h>
#include <unistd.h>

struct proto proto_v4 = {proc_v4, send_v4, NULL, NULL, 0, IPPROTO_ICMP};

#ifdef IPV6
struct proto proto_v6 = {proc_v6, send_v6, NULL, NULL, 0, IPPROTO_ICMPV6};
#endif

int datalen = 56; /* data that goes with ICMP echo request */
int option_emit_audio = 0;
double option_interval = 1;
int option_maxsend = 0;
int option_ttl = 0;
int option_broadcast_allowed = 0;
int option_only_analytics = 0;
int option_debug = 0;
int option_dont_route = 0;
int option_buffer_size = 0;
int status_will_be_broadcasting = 0;
int option_protflag = 0;
int option_mtu = 0;
int option_timestamp = 0;
int option_flush = 0;
int option_adaptive = 0;
int option_mark = 0;
int option_deadline = 0;
const int const_allow_hdr = 1;
int halt_operation = 0;

double stats_sent = 0;
double stats_recv = 0;
double stats_total_delay = 0.0f;

int main(int argc, char **argv) {
  int c;
  struct addrinfo *ai;

  extern char *optarg;
  extern int optind, optopt;

  int optioncnt = 0;
  int optionextra = 0;

  while ((c = getopt(argc, argv, "46aAbB:c:dDfhi:qrs:t:vm:MVw:")) != -1) {
    optioncnt++;
    switch (c) {
    case '4':
      option_protflag = 4;
      break;
    case '6':
      option_protflag = 6;
      break;
    case 'a':
      option_emit_audio = 1;
      break;
    case 'A':
      option_adaptive = 1;
      break;
    case 'b':
      // Ping broadcast
      option_broadcast_allowed = 1;
      break;
    case 'B':
      option_buffer_size = atoi(optarg);
      if (option_buffer_size <= 0) {
        err_quit("Error when handling option %c: cannot be a negative value!",
                 optopt);
      }
      break;
    case 'c':
      // Terminate after certain count
      option_maxsend = atoi(optarg);
      if (option_maxsend < 0) {
        err_quit("Error when handling option %c: cannot be a negative value!",
                 optopt);
      }
      optionextra++;
      break;
    case 'd':
      option_timestamp = 1;
      break;
    case 'D':
      option_debug = 1;
      break;
    case 'f':
      option_flush = 1;
      setbuf(stdout, NULL);
      break;
    case 'h':
      // Helpstring
      printf("Ping-octo-bassoon v1.2 Help\n\
Usage\n\
\tusage: ping [options] <hostname>\n\n\
Options\n\
\t<hostname>\tdns name or ip address\n\
\t-4\t\tIPv4 Only\n\
\t-6\t\tIPv6 Only\n\
\t-a\t\tMake audible cue when receiving\n\
\t-A\t\tSort of Adaptive Ping\n\
\t-b\t\tAllow broadcast\n\
\t-B <size>\tSet Buffer Size\n\
\t-c <maxsend>\tMax send count before termination\n\
\t-d\t\tSet Debug On\n\
\t-D\t\tPrint Timestamp\n\
\t-f\t\tFlood ping. For every ECHO_REQUEST sent a period “.” is printed, while for every ECHO_REPLY received a backspace is printed. \n\
\t-h\t\tShow this message\n\
\t-i <interval>\tSend interval\n\
\t-m <mark>\tMarking packet\n\
\t-M\t\tMTU Enable\n\
\t-q\t\tOnly output results when finishing / terminating\n\
\t-r\t\tDont Route\n\
\t-s <sendsize>\tSet packet size\n\
\t-t <ttl>\tSet TTL\n\
\t-v\t\tVerbose\n\
\t-V\t\tPrint Version\n\
\t-w <deadline>\tTermination time by seconds\n");

      exit(0);

    case 'i':
      // Send Interval
      option_interval = atof(optarg);
      if (option_interval < 0) {
        err_quit("Error when handling option %c: cannot be a negative value!",
                 optopt);
      }
      optionextra++;
      break;
    case 'm':
      option_mark = atoi(optarg);
      optionextra++;
      break;

    case 'M':
      option_mtu = 1;
      break;
    case 'q':
      // Only show analytics
      option_only_analytics = 1;
      if (verbose > 0) {
        err_quit("Conflict when handling option %c: -v cant be used with -q!",
                 optopt);
      }
      break;
    case 'r':
      option_dont_route = 1;
      break;
    case 's':
      datalen = atoi(optarg);
      if (datalen < 0) {
        err_quit("Error when handling option %c: cannot be a negative value!",
                 optopt);
      }
      optionextra++;
      break;

    case 't':
      option_ttl = atoi(optarg);
      if (option_ttl < 0) {
        err_quit("Error when handling option %c: cannot be a negative value!",
                 optopt);
      }
      optionextra++;
      break;

    case 'v':
      verbose++;
      if (option_only_analytics) {
        err_quit("Conflict when handling option %c: -v cant be used with -q!",
                 optopt);
      }
      break;



    case 'V':
      printf("Ping-octo-bassoon v1.1 Help\n");
      exit(0);
    case 'w':
      option_deadline = atoi(optarg);
      if (option_deadline <= 0) {
        err_quit("Error when handling option %c: cannot be a negative value!",
                 optopt);
      }
      break;
    case '?':
      err_quit("unrecognized option: %c", optopt);
      return (0);

    case ':':
      err_quit("Missing Argument: %c", c);
      break;

    default:
      optioncnt--;
    }
  }

  if (optind != argc - 1)
    err_quit("usage: ping [options] <hostname>. add -h for more info.");

  host = argv[optind];

  pid = getpid();
  signal(SIGALRM, sig_alrm);

  switch (option_protflag) {
  case 6: {
    ai = host_serv(host, NULL, AF_INET6, 0);
    if (ai == NULL) {
      err_quit("An IPv6 Address does not exist unfortunately.");
    }
    break;
  }
  case 4:
  default: {
    ai = host_serv(host, NULL, 0, 0);
    if (ai == NULL) {
      err_quit("An IPv4 Address does not exist unfortunately.");
    }
    break;
  }
  }

  printf("ping %s (%s): %d data bytes\n", ai->ai_canonname,
         Sock_ntop_host(ai->ai_addr, ai->ai_addrlen), datalen);

  /* initialize according to protocol */
  if (ai->ai_family == AF_INET) {

    pr = &proto_v4;
    // printf("is ipv4!\n");
    if (strcmp(host, "255.255.255.255") == 0) {
      if (option_broadcast_allowed > 0) {
        status_will_be_broadcasting = 1;
      } else {
        err_quit("ping: Do you want to ping broadcast? Then -b. If not, check "
                 "your local firewall rules");
      }
    }
#ifdef IPV6
  } else if (ai->ai_family == AF_INET6) {
    pr = &proto_v6;
    // printf("is ipv6!\n");
    if (IN6_IS_ADDR_V4MAPPED(
            &(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr)))
      err_quit("cannot ping IPv4-mapped IPv6 address");
#endif
  } else
    err_quit("unknown address family %d", ai->ai_family);

  pr->sasend = ai->ai_addr;
  pr->sarecv = calloc(1, ai->ai_addrlen);
  pr->salen = ai->ai_addrlen;

  readloop();

  exit(0);
}

void proc_v4(char *ptr, ssize_t len, struct timeval *tvrecv) {
  int hlen1, icmplen;
  double rtt;
  struct ip *ip;
  struct icmp *icmp;
  struct timeval *tvsend;

  ip = (struct ip *)ptr;  /* start of IP header */
  hlen1 = ip->ip_hl << 2; /* length of IP header */

  icmp = (struct icmp *)(ptr + hlen1); /* start of ICMP header */
  if ((icmplen = len - hlen1) < 8)
    err_quit("icmplen (%d) < 8", icmplen);

  if (icmp->icmp_type == ICMP_ECHOREPLY) {
    if (icmp->icmp_id != pid && option_broadcast_allowed != 2)
      return; /* not a response to our ECHO_REQUEST */
    if (icmplen < 16)
      err_quit("icmplen (%d) < 16", icmplen);

    tvsend = (struct timeval *)icmp->icmp_data;
    double time = tvrecv->tv_sec + (tvrecv->tv_usec / 1000000.0);
    tv_sub(tvrecv, tvsend);
    rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

    stats_total_delay += rtt;
    stats_recv++;
    if (option_emit_audio) {
      putchar('\a');
    }
    if (option_adaptive) {
      option_interval =
          ((rtt / 1000 + 0.005) > 0.2) ? (rtt / 1000 + 0.005) : 0.2;
    }
    if (option_flush == 1) {
      printf("\b");
    } else if (!option_only_analytics) {
      if (option_timestamp) {
        printf("Now is %lf, ", time);
      }
      printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n", icmplen,
             Sock_ntop_host(pr->sarecv, pr->salen), icmp->icmp_seq, ip->ip_ttl,
             rtt);
    }

  } else if (verbose) {
    printf("  %d bytes from %s: type = %d, code = %d\n", icmplen,
           Sock_ntop_host(pr->sarecv, pr->salen), icmp->icmp_type,
           icmp->icmp_code);
  }
}

void proc_v6(char *ptr, ssize_t len, struct timeval *tvrecv) {
#ifdef IPV6

  int hlen1, icmp6len;
  double rtt;
  struct ip6_hdr *ip6;
  struct icmp6_hdr *icmp6;
  struct timeval *tvsend;

  /*
  ip6 = (struct ip6_hdr *) ptr;		// start of IPv6 header
  hlen1 = sizeof(struct ip6_hdr);
  if (ip6->ip6_nxt != IPPROTO_ICMPV6)
          err_quit("next header not IPPROTO_ICMPV6");

  icmp6 = (struct icmp6_hdr *) (ptr + hlen1);
  if ( (icmp6len = len - hlen1) < 8)
          err_quit("icmp6len (%d) < 8", icmp6len);
  */
  // ip6 = (struct ip6_hdr *)ptr;
  // hlen1 = sizeof(struct ip6_hdr);
  icmp6 = (struct icmp6_hdr *)ptr;
  if ((icmp6len = len) < 8) // len-40
    err_quit("icmp6len (%d) < 8", icmp6len);

  if (icmp6->icmp6_type == ICMP6_ECHO_REPLY) {

    if (icmp6->icmp6_id != pid)
      return; /* not a response to our ECHO_REQUEST */
    if (icmp6len < 16)
      err_quit("icmp6len (%d) < 16", icmp6len);

    tvsend = (struct timeval *)(icmp6 + 1);
    double time = tvrecv->tv_sec + (tvrecv->tv_usec / 1000000.0);
    tv_sub(tvrecv, tvsend);
    rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
    stats_total_delay += rtt;
    stats_recv++;

    if (option_emit_audio)
      putchar('\a');
    if (option_adaptive == 1) {
      option_interval =
          ((rtt / 1000 + 0.005) > 0.2) ? (rtt / 1000 + 0.005) : 0.2;
    }
    if (option_flush == 1) {
      putchar('\b');
    } else if (!option_only_analytics) {
      if (option_timestamp) {
        printf("Now is %lf, ", time);
      }
      printf("%d bytes from %s: seq=%u, rtt=%.3f ms\n", icmp6len,
             Sock_ntop_host(pr->sarecv, pr->salen), icmp6->icmp6_seq, rtt);
      // printf("%d bytes from %s: seq=%u, hlim=%d, rtt=%.3f ms\n", icmp6len,
      //        Sock_ntop_host(pr->sarecv, pr->salen), icmp6->icmp6_seq,
      //        ip6->ip6_hlim, rtt);
    }

  } else if (verbose) {
    printf("  %d bytes from %s: type = %d, code = %d\n", icmp6len,
           Sock_ntop_host(pr->sarecv, pr->salen), icmp6->icmp6_type,
           icmp6->icmp6_code);
  }

#endif /* IPV6 */
}

unsigned short in_cksum(unsigned short *addr, int len) {
  int nleft = len;
  int sum = 0;
  unsigned short *w = addr;
  unsigned short answer = 0;

  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  /* 4mop up an odd byte, if necessary */
  if (nleft == 1) {
    *(unsigned char *)(&answer) = *(unsigned char *)w;
    sum += answer;
  }

  /* 4add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
  sum += (sum >> 16);                 /* add carry */
  answer = ~sum;                      /* truncate to 16 bits */
  return (answer);
}

void send_v4(void) {
  int len;
  struct icmp *icmp;

  icmp = (struct icmp *)sendbuf;
  icmp->icmp_type = ICMP_ECHO;
  icmp->icmp_code = 0;
  icmp->icmp_id = pid;
  icmp->icmp_seq = nsent++;
  gettimeofday((struct timeval *)icmp->icmp_data, NULL);

  len = 8 + datalen; /* checksum ICMP header and data */
  icmp->icmp_cksum = 0;
  icmp->icmp_cksum = in_cksum((u_short *)icmp, len);

  if (option_broadcast_allowed == 1 && status_will_be_broadcasting) {

    // Set destination IP address to broadcast address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr =
        htonl(INADDR_BROADCAST); // Use INADDR_BROADCAST for broadcast address

    if (sendto(sockfd, sendbuf, len, 0, (struct sockaddr *)&dest_addr,
               sizeof(dest_addr)) < 0) {
      perror("sendto (broadcast) failed");
    } else if (option_flush == 1) {
      printf(".");
    }
  } else {
    if (sendto(sockfd, sendbuf, len, 0, (struct sockaddr *)pr->sasend,
               pr->salen) < 0) {
      perror("sendto (regular) failed");
    } else if (option_flush == 1) {
      printf(".");
    }
  }
}

void send_v6() {
#ifdef IPV6
  int len;
  struct icmp6_hdr *icmp6;

  icmp6 = (struct icmp6_hdr *)sendbuf;
  icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
  icmp6->icmp6_code = 0;
  icmp6->icmp6_id = pid;
  icmp6->icmp6_seq = nsent++;
  gettimeofday((struct timeval *)(icmp6 + 1), NULL);

  len = 8 + datalen; /* 8-byte ICMPv6 header */

  if (sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen) < 0) {
    perror("sendto (ipv6) failed");
  } else if (option_flush) {
    printf(".");
  }
  /* kernel calculates and stores checksum for us */
#endif /* IPV6 */
}

void summarize_on_halt() {
  halt_operation = 1;
  printf("\n%.0f sent, %.0f received, avg rtt = %.3f ms\n", stats_sent,
         stats_recv, stats_total_delay / stats_recv);
  exit(0);
}

void readloop(void) {
  int size;
  char recvbuf[BUFSIZE];
  socklen_t len;
  ssize_t n;
  struct timeval tval;
  struct timeval tval_now;
  struct timeval tval_begin;

  sockfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
  if (sockfd < 0) {
    printf("Socket went wrong: Probably missing superuser priv.\n");
    err_sys("socket creation failed");
  }

  if (option_ttl != 0) {
    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &option_ttl,
                   sizeof(option_ttl)) < 0) {
      perror("setsockopt");
    }
  }

  if (option_debug != 0) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, &option_debug,
                   sizeof(option_debug)) < 0) {
      perror("setsockopt");
    }
  }

  if (option_dont_route != 0) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, &option_dont_route,
                   sizeof(option_dont_route)) < 0) {
      perror("setsockopt");
    }
  }
  if (option_buffer_size != 0) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &option_buffer_size,
                   sizeof(option_buffer_size)) < 0) {
      perror("setsockopt");
    }
  }
  if (option_mtu != 0) {
    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &option_mtu,
                   sizeof(option_mtu)) < 0) {
      perror("setsockopt");
    }
  }
  if (option_mark != 0) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_MARK, &option_mark,
                   sizeof(option_mark)) < 0) {
      perror("setsockopt");
    }
  }

  if (option_broadcast_allowed != 0) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &option_broadcast_allowed,
                   sizeof(option_broadcast_allowed)) < 0) {
      perror("setsockopt (SO_BROADCAST) failed");
      return;
    }
  }

  // if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &const_allow_hdr,
  // sizeof(const_allow_hdr)) < 0) {
  //       perror("setsockopt");
  //   }

  setuid(getuid()); /* don't need special permissions any more */
  gettimeofday(&tval_begin, NULL);
  size = 60 * 1024; /* OK if setsockopt fails */
  setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

  sig_alrm(SIGALRM); /* send first packet */

  signal(SIGINT, summarize_on_halt);

  for (;;) {
    len = pr->salen;
    n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);

    if (n < 0) {
      if (errno == EINTR) {

        continue;
      } else
        err_sys("recvfrom error");
    }

    gettimeofday(&tval, NULL);
    (*pr->fproc)(recvbuf, n, &tval);
    gettimeofday(&tval_now, NULL);
    tv_sub(&tval_now, &tval_begin);
    // printf("elapsed %ld max %d", tval_now.tv_sec, option_deadline);
    if ((tval_now.tv_sec >= option_deadline) && option_deadline != 0) {
      summarize_on_halt();
    }
    if (halt_operation == 1)
      break;
  }
  summarize_on_halt();
}

void sig_alrm(int signo) {
  (*pr->fsend)();
  if (halt_operation == 0) {
    if (stats_sent >= option_maxsend - 1 && option_maxsend > 0) {
      stats_sent++;
      halt_operation = 1;
      return;
    }
    stats_sent++;
    long sec = (int)option_interval;
    long usec = (long)((option_interval - sec) * 1e6);

    struct itimerval timer;
    timer.it_interval.tv_sec = sec;
    timer.it_interval.tv_usec = usec;
    timer.it_value.tv_sec = sec;
    timer.it_value.tv_usec = usec;
    if (setitimer(ITIMER_REAL, &timer, NULL) != 0) {
      err_quit("Timer failed!");
    }
  }
  return; /* probably interrupts recvfrom() */
}

void tv_sub(struct timeval *out, struct timeval *in) {
  if ((out->tv_usec -= in->tv_usec) < 0) { /* out -= in */
    --out->tv_sec;
    out->tv_usec += 1000000;
  }
  out->tv_sec -= in->tv_sec;
}

char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen) {
  static char str[128]; /* Unix domain is largest */

  switch (sa->sa_family) {
  case AF_INET: {
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;

    if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
      return (NULL);
    return (str);
  }

#ifdef IPV6
  case AF_INET6: {
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

    if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
      return (NULL);
    return (str);
  }
#endif

#ifdef HAVE_SOCKADDR_DL_STRUCT
  case AF_LINK: {
    struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;

    if (sdl->sdl_nlen > 0)
      snprintf(str, sizeof(str), "%*s", sdl->sdl_nlen, &sdl->sdl_data[0]);
    else
      snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
    return (str);
  }
#endif
  default:
    snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
             sa->sa_family, salen);
    return (str);
  }
  return (NULL);
}

char *Sock_ntop_host(const struct sockaddr *sa, socklen_t salen) {
  char *ptr;

  if ((ptr = sock_ntop_host(sa, salen)) == NULL)
    err_sys("sock_ntop_host error"); /* inet_ntop() sets errno */
  return (ptr);
}

struct addrinfo *host_serv(const char *host, const char *serv, int family,
                           int socktype) {
  int n;
  struct addrinfo hints, *res;

  bzero(&hints, sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME; /* always return canonical name */
  hints.ai_family = family;      /* AF_UNSPEC, AF_INET, AF_INET6, etc. */
  hints.ai_socktype = socktype;  /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

  if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
    return (NULL);

  return (res); /* return pointer to first on linked list */
}
/* end host_serv */

static void err_doit(int errnoflag, int level, const char *fmt, va_list ap) {
  int errno_save, n;
  char buf[MAXLINE];

  errno_save = errno; /* value caller might want printed */
#ifdef HAVE_VSNPRINTF
  vsnprintf(buf, sizeof(buf), fmt, ap); /* this is safe */
#else
  vsprintf(buf, fmt, ap); /* this is not safe */
#endif
  n = strlen(buf);
  if (errnoflag)
    snprintf(buf + n, sizeof(buf) - n, ": %s", strerror(errno_save));
  strcat(buf, "\n");

  if (daemon_proc) {
    syslog(level, "%s", buf);
    /* syslog(level, buf); */
  } else {
    fflush(stdout); /* in case stdout and stderr are the same */
    fputs(buf, stderr);
    fflush(stderr);
  }
  return;
}

/* Fatal error unrelated to a system call.
 * Print a message and terminate. */

void err_quit(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  err_doit(0, LOG_ERR, fmt, ap);
  va_end(ap);
  exit(1);
}

/* Fatal error related to a system call.
 * Print a message and terminate. */

void err_sys(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  err_doit(1, LOG_ERR, fmt, ap);
  va_end(ap);
  exit(1);
}

/*
 * getopt是由Unix标准库提供的函数，查看命令man 3 getopt。
 *
 * getopt函数的参数：
 * 参数argc和argv：通常是从main的参数直接传递而来，argc是参数的数量，
 *                 argv是一个常量字符串数组的地址。
 * 参数optstring：一个包含正确选项字符的字符串，如果一个字符后面有冒号，
                                  那么这个选项在传递参数时就需要跟着一个参数。

 * 外部变量：
 * char *optarg：如果有参数，则包含当前选项参数字符串
 * int optind：argv的当前索引值。当getopt函数在while循环中使用时，
 *             剩下的字符串为操作数，下标从optind到argc-1。
 * int opterr：这个变量非零时，getopt()函数为“无效选项”和“缺少参数选项，
 *             并输出其错误信息。
 * int optopt：当发现无效选项字符之时，getopt()函数或返回 \’ ? \’ 字符，
 *             或返回字符 \’ : \’ ，并且optopt包含了所发现的无效选项字符。
 */
