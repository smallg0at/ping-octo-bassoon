# Ping

This is a teamwork for BJUT's Networking Course Design.

## Usage

```bash
make
sudo ./ping [options] <ip>
```

## Major Improvements on the previously given code

- IPv6 functionality has been restored and can actually be used now without segfaulting. Try this: `sudo ./ping -6 ipv6.test-ipv6.com`
- Enabled rapid fire ability by tinkering with the timer system. this means you could now have a < 1s interval.

## Help
```
Ping-octo-bassoon v1.2 Help
Usage
        usage: ping [options] <hostname>

Options
        <hostname>      dns name or ip address
        -4              IPv4 Only
        -6              IPv6 Only
        -a              Make audible cue when receiving
        -A              Sort of Adaptive Ping
        -b              Allow broadcast
        -B <size>       Set Buffer Size
        -c <maxsend>    Max send count before termination
        -d              Set Debug On
        -D              Print Timestamp
        -f              Flood ping. For every ECHO_REQUEST sent a period “.” is printed, while for every ECHO_REPLY received a backspace is printed. 
        -h              Show this message
        -i <interval>   Send interval
        -m <mark>       Marking packet
        -M <option>     MTU Stats. Allowed values: dont, want, do, probe.
        -q              Only output results when finishing / terminating
        -r              Dont Route
        -s <sendsize>   Set packet size
        -t <ttl>        Set TTL
        -v              Verbose
        -V              Print Version
        -w <deadline>   Termination time by seconds
```
