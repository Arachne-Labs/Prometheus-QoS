/* Modified by: xChaos, 20130114 */

#include "cll1-0.6.2.h"

/* ======= Help screen is hopefuly self-documenting part of code :-) ======= */

void help(void)
{
 puts("Command line switches:\n\
\n\
-d   Dry run (preview tc and iptables commands on stdout)\n\
-r   Run (reset all statistics and start shaping - daily usage)\n\
-p   just generate Preview of data transfer statistics and exit (after -r)\n\
-s   start Shaping FUP limits (keeps data transfer stat like -p) (after -r)\n\
-n   run Now (like -r delay - overrides qos-free-delay keyword, after boot)\n\
-f   just Flush iptables and tc classes and exit (stop shaping, no QiS)\n\
-9   emergency iptables flush (like -f, but dumps data transfer statistics)\n\
\n\
-c filename  force alternative /etc/prometheus/prometheus.conf filename\n\
-h filename  force alternative /etc/hosts filename (overrides hosts keyword)\n\
-l Mmm YYYY  generate HTML summary of Logged traffic (Mmm=Jan-Dec) (and exit)\n\
-m           generate HTML summary of traffic for yesterday's Month (and exit)\n\
-y           generate HTML summary of traffic for yesterday's Year (and exit)\n\
-? --help    show this help scree (and exit)\n\
-v --version show Version number of this utility (and exit)\n");
}
