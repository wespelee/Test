<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>syslogd.c</title>
<style type="text/css">
.enscript-comment { font-style: italic; color: rgb(178,34,34); }
.enscript-function-name { font-weight: bold; color: rgb(0,0,255); }
.enscript-variable-name { font-weight: bold; color: rgb(184,134,11); }
.enscript-keyword { font-weight: bold; color: rgb(160,32,240); }
.enscript-reference { font-weight: bold; color: rgb(95,158,160); }
.enscript-string { font-weight: bold; color: rgb(188,143,143); }
.enscript-builtin { font-weight: bold; color: rgb(218,112,214); }
.enscript-type { font-weight: bold; color: rgb(34,139,34); }
.enscript-highlight { text-decoration: underline; color: 0; }
</style>
</head>
<body id="top">
<h1 style="margin:8px;" id="f1">syslogd.c&nbsp;&nbsp;&nbsp;<span style="font-weight: normal; font-size: 0.5em;">[<a href="?txt">plain text</a>]</span></h1>
<hr/>
<div></div>
<pre>
<span class="enscript-comment">/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * &quot;Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at <a href="http://www.apple.com/publicsource">http://www.apple.com/publicsource</a> and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.&quot;
 * 
 * @APPLE_LICENSE_HEADER_END@
 */</span>
<span class="enscript-comment">/*
 * Copyright (c) 1983, 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */</span>

#<span class="enscript-reference">ifndef</span> <span class="enscript-variable-name">lint</span>
<span class="enscript-type">static</span> <span class="enscript-type">char</span> copyright[] =
<span class="enscript-string">&quot;@(#) Copyright (c) 1983, 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n&quot;</span>;
#<span class="enscript-reference">endif</span> <span class="enscript-comment">/* not lint */</span>

#<span class="enscript-reference">ifndef</span> <span class="enscript-variable-name">lint</span>
<span class="enscript-type">static</span> <span class="enscript-type">char</span> sccsid[] = <span class="enscript-string">&quot;@(#)syslogd.c	8.3 (Berkeley) 4/4/94&quot;</span>;
#<span class="enscript-reference">endif</span> <span class="enscript-comment">/* not lint */</span>

<span class="enscript-comment">/*
 *  syslogd -- log system messages
 *
 * This program implements a system log. It takes a series of lines.
 * Each line may have a priority, signified as &quot;&lt;n&gt;&quot; as
 * the first characters of the line.  If this is
 * not present, a default priority is used.
 *
 * To kill syslogd, send a signal 15 (terminate).  A signal 1 (hup) will
 * cause it to reread its configuration file.
 *
 * Defined Constants:
 *
 * MAXLINE -- the maximimum line length that can be handled.
 * DEFUPRI -- the default priority for user messages
 * DEFSPRI -- the default priority for kernel messages
 *
 * Author: Eric Allman
 * extensive changes by Ralph Campbell
 * more extensive changes by Eric Allman (again)
 */</span>

#<span class="enscript-reference">define</span>	<span class="enscript-variable-name">MAXLINE</span>		1024		<span class="enscript-comment">/* maximum line length */</span>
#<span class="enscript-reference">define</span>	<span class="enscript-variable-name">MAXSVLINE</span>	120		<span class="enscript-comment">/* maximum saved line length */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">DEFUPRI</span>		(LOG_USER|LOG_NOTICE)
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">DEFSPRI</span>		(LOG_KERN|LOG_CRIT)
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">TIMERINTVL</span>	30		<span class="enscript-comment">/* interval for checking flush, mark */</span>

#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/param.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/ioctl.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/stat.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/wait.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/socket.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/msgbuf.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/uio.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/un.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/time.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/resource.h&gt;</span>

#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;netinet/in.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;netdb.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;arpa/inet.h&gt;</span>

#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;ctype.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;errno.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;fcntl.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;setjmp.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;signal.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;stdio.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;stdlib.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;string.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;unistd.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;utmp.h&gt;</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&quot;pathnames.h&quot;</span>

#<span class="enscript-reference">define</span> <span class="enscript-variable-name">SYSLOG_NAMES</span>
#<span class="enscript-reference">include</span> <span class="enscript-string">&lt;sys/syslog.h&gt;</span>

<span class="enscript-type">char</span>	*LogName = _PATH_LOG;
<span class="enscript-type">char</span>	*ConfFile = _PATH_LOGCONF;
<span class="enscript-type">char</span>	*PidFile = _PATH_LOGPID;
<span class="enscript-type">char</span>	ctty[] = _PATH_CONSOLE;

#<span class="enscript-reference">define</span> <span class="enscript-function-name">FDMASK</span>(fd)	(1 &lt;&lt; (fd))

#<span class="enscript-reference">define</span>	<span class="enscript-variable-name">dprintf</span>		if (Debug) printf

#<span class="enscript-reference">define</span> <span class="enscript-variable-name">MAXUNAMES</span>	20	<span class="enscript-comment">/* maximum number of user names */</span>

<span class="enscript-comment">/*
 * Flags to logmsg().
 */</span>

#<span class="enscript-reference">define</span> <span class="enscript-variable-name">IGN_CONS</span>	0x001	<span class="enscript-comment">/* don't print on console */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">SYNC_FILE</span>	0x002	<span class="enscript-comment">/* do fsync on file after printing */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">ADDDATE</span>		0x004	<span class="enscript-comment">/* add a date to the message */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">MARK</span>		0x008	<span class="enscript-comment">/* this message is a mark */</span>

<span class="enscript-comment">/*
 * This structure represents the files that will have log
 * copies printed.
 */</span>

<span class="enscript-type">struct</span> filed {
	<span class="enscript-type">struct</span>	filed *f_next;		<span class="enscript-comment">/* next in linked list */</span>
	<span class="enscript-type">short</span>	f_type;			<span class="enscript-comment">/* entry type, see below */</span>
	<span class="enscript-type">short</span>	f_file;			<span class="enscript-comment">/* file descriptor */</span>
	time_t	f_time;			<span class="enscript-comment">/* time this was last written */</span>
	u_char	f_pmask[LOG_NFACILITIES+1];	<span class="enscript-comment">/* priority mask */</span>
	<span class="enscript-type">union</span> {
		<span class="enscript-type">char</span>	f_uname[MAXUNAMES][UT_NAMESIZE+1];
		<span class="enscript-type">struct</span> {
			<span class="enscript-type">char</span>	f_hname[MAXHOSTNAMELEN+1];
			<span class="enscript-type">struct</span> sockaddr_in	f_addr;
		} f_forw;		<span class="enscript-comment">/* forwarding address */</span>
		<span class="enscript-type">char</span>	f_fname[MAXPATHLEN];
	} f_un;
	<span class="enscript-type">char</span>	f_prevline[MAXSVLINE];		<span class="enscript-comment">/* last message logged */</span>
	<span class="enscript-type">char</span>	f_lasttime[16];			<span class="enscript-comment">/* time of last occurrence */</span>
	<span class="enscript-type">char</span>	f_prevhost[MAXHOSTNAMELEN+1];	<span class="enscript-comment">/* host from which recd. */</span>
	<span class="enscript-type">int</span>	f_prevpri;			<span class="enscript-comment">/* pri of f_prevline */</span>
	<span class="enscript-type">int</span>	f_prevlen;			<span class="enscript-comment">/* length of f_prevline */</span>
	<span class="enscript-type">int</span>	f_prevcount;			<span class="enscript-comment">/* repetition cnt of prevline */</span>
	<span class="enscript-type">int</span>	f_repeatcount;			<span class="enscript-comment">/* number of &quot;repeated&quot; msgs */</span>
};

<span class="enscript-comment">/*
 * Intervals at which we flush out &quot;message repeated&quot; messages,
 * in seconds after previous message is logged.  After each flush,
 * we move to the next interval until we reach the largest.
 */</span>
<span class="enscript-type">int</span>	repeatinterval[] = { 30, 120, 600 };	<span class="enscript-comment">/* # of secs before flush */</span>
#<span class="enscript-reference">define</span>	<span class="enscript-variable-name">MAXREPEAT</span> ((sizeof(repeatinterval) / sizeof(repeatinterval[0])) - 1)
#<span class="enscript-reference">define</span>	<span class="enscript-function-name">REPEATTIME</span>(f)	((f)-&gt;f_time + repeatinterval[(f)-&gt;f_repeatcount])
#<span class="enscript-reference">define</span>	<span class="enscript-function-name">BACKOFF</span>(f)	{ if (++(f)-&gt;f_repeatcount &gt; MAXREPEAT) \
				 (f)-&gt;f_repeatcount = MAXREPEAT; \
			}

<span class="enscript-comment">/* values for f_type */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">F_UNUSED</span>	0		<span class="enscript-comment">/* unused entry */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">F_FILE</span>		1		<span class="enscript-comment">/* regular file */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">F_TTY</span>		2		<span class="enscript-comment">/* terminal */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">F_CONSOLE</span>	3		<span class="enscript-comment">/* console terminal */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">F_FORW</span>		4		<span class="enscript-comment">/* remote machine */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">F_USERS</span>		5		<span class="enscript-comment">/* list of users */</span>
#<span class="enscript-reference">define</span> <span class="enscript-variable-name">F_WALL</span>		6		<span class="enscript-comment">/* everyone logged on */</span>

<span class="enscript-type">char</span>	*TypeNames[7] = {
	<span class="enscript-string">&quot;UNUSED&quot;</span>,	<span class="enscript-string">&quot;FILE&quot;</span>,		<span class="enscript-string">&quot;TTY&quot;</span>,		<span class="enscript-string">&quot;CONSOLE&quot;</span>,
	<span class="enscript-string">&quot;FORW&quot;</span>,		<span class="enscript-string">&quot;USERS&quot;</span>,	<span class="enscript-string">&quot;WALL&quot;</span>
};

<span class="enscript-type">struct</span>	filed *Files;
<span class="enscript-type">struct</span>	filed consfile;

<span class="enscript-type">int</span>	Debug = 0;		<span class="enscript-comment">/* debug flag */</span>
<span class="enscript-type">int</span>	Insecure = 0;		<span class="enscript-comment">/* insecure flag */</span>
<span class="enscript-type">char</span>	LocalHostName[MAXHOSTNAMELEN+1];	<span class="enscript-comment">/* our hostname */</span>
<span class="enscript-type">char</span>	*LocalDomain;		<span class="enscript-comment">/* our local domain name */</span>
<span class="enscript-type">int</span>	InetInuse = 0;		<span class="enscript-comment">/* non-zero if INET sockets are being used */</span>
<span class="enscript-type">int</span>	finet;			<span class="enscript-comment">/* Internet datagram socket */</span>
<span class="enscript-type">int</span>	LogPort;		<span class="enscript-comment">/* port number for INET connections */</span>
<span class="enscript-type">int</span>	Initialized = 0;	<span class="enscript-comment">/* set when we have initialized ourselves */</span>
<span class="enscript-type">int</span>	MarkInterval = 20 * 60;	<span class="enscript-comment">/* interval between marks in seconds */</span>
<span class="enscript-type">int</span>	MarkSeq = 0;		<span class="enscript-comment">/* mark sequence number */</span>

<span class="enscript-type">void</span>	cfline __P((<span class="enscript-type">char</span> *, <span class="enscript-type">struct</span> filed *));
<span class="enscript-type">char</span>   *cvthname <span class="enscript-function-name">__P</span>((<span class="enscript-type">struct</span> sockaddr_in *));
<span class="enscript-type">int</span>	decode __P((<span class="enscript-type">const</span> <span class="enscript-type">char</span> *, CODE *));
<span class="enscript-type">void</span>	die __P((<span class="enscript-type">int</span>));
<span class="enscript-type">void</span>	domark __P((<span class="enscript-type">int</span>));
<span class="enscript-type">void</span>	fprintlog __P((<span class="enscript-type">struct</span> filed *, <span class="enscript-type">int</span>, <span class="enscript-type">char</span> *));
<span class="enscript-type">void</span>	init __P((<span class="enscript-type">int</span>));
<span class="enscript-type">void</span>	logerror __P((<span class="enscript-type">char</span> *));
<span class="enscript-type">void</span>	logmsg __P((<span class="enscript-type">int</span>, <span class="enscript-type">char</span> *, <span class="enscript-type">char</span> *, <span class="enscript-type">int</span>));
<span class="enscript-type">void</span>	printline __P((<span class="enscript-type">char</span> *, <span class="enscript-type">char</span> *));
<span class="enscript-type">void</span>	printsys __P((<span class="enscript-type">char</span> *));
<span class="enscript-type">void</span>	reapchild __P((<span class="enscript-type">int</span>));
<span class="enscript-type">char</span>   *ttymsg <span class="enscript-function-name">__P</span>((<span class="enscript-type">struct</span> iovec *, <span class="enscript-type">int</span>, <span class="enscript-type">char</span> *, <span class="enscript-type">int</span>));
<span class="enscript-type">void</span>	usage __P((<span class="enscript-type">void</span>));
<span class="enscript-type">void</span>	wallmsg __P((<span class="enscript-type">struct</span> filed *, <span class="enscript-type">struct</span> iovec *));

<span class="enscript-type">int</span>
<span class="enscript-function-name">main</span>(argc, argv)
	<span class="enscript-type">int</span> argc;
	<span class="enscript-type">char</span> *argv[];
{
	<span class="enscript-type">int</span> ch, funix, i, inetm, fklog, klogm, len;
	<span class="enscript-type">struct</span> sockaddr_un sunx, fromunix;
	<span class="enscript-type">struct</span> sockaddr_in sin, frominet;
	FILE *fp;
	<span class="enscript-type">char</span> *p, line[MSG_BSIZE + 1];

	<span class="enscript-keyword">while</span> ((ch = getopt(argc, argv, <span class="enscript-string">&quot;duf:m:p:&quot;</span>)) != EOF)
		<span class="enscript-keyword">switch</span>(ch) {
		<span class="enscript-keyword">case</span> <span class="enscript-string">'d'</span>:		<span class="enscript-comment">/* debug */</span>
			Debug++;
			<span class="enscript-keyword">break</span>;
		<span class="enscript-keyword">case</span> <span class="enscript-string">'u'</span>:		<span class="enscript-comment">/* insecure */</span>
			Insecure++;
			<span class="enscript-keyword">break</span>;
		<span class="enscript-keyword">case</span> <span class="enscript-string">'f'</span>:		<span class="enscript-comment">/* configuration file */</span>
			ConfFile = optarg;
			<span class="enscript-keyword">break</span>;
		<span class="enscript-keyword">case</span> <span class="enscript-string">'m'</span>:		<span class="enscript-comment">/* mark interval */</span>
			MarkInterval = atoi(optarg) * 60;
			<span class="enscript-keyword">break</span>;
		<span class="enscript-keyword">case</span> <span class="enscript-string">'p'</span>:		<span class="enscript-comment">/* path */</span>
			LogName = optarg;
			<span class="enscript-keyword">break</span>;
		<span class="enscript-keyword">case</span> <span class="enscript-string">'?'</span>:
		<span class="enscript-reference">default</span>:
			usage();
		}
	<span class="enscript-keyword">if</span> ((argc -= optind) != 0)
		usage();

	<span class="enscript-keyword">if</span> (!Debug)
		(<span class="enscript-type">void</span>)daemon(0, 0);
	<span class="enscript-keyword">else</span>
		setlinebuf(stdout);

	consfile.f_type = F_CONSOLE;
	(<span class="enscript-type">void</span>)strcpy(consfile.f_un.f_fname, ctty);
	(<span class="enscript-type">void</span>)gethostname(LocalHostName, <span class="enscript-keyword">sizeof</span>(LocalHostName));
	<span class="enscript-keyword">if</span> ((p = strchr(LocalHostName, <span class="enscript-string">'.'</span>)) != NULL) {
		*p++ = <span class="enscript-string">'\0'</span>;
		LocalDomain = p;
	} <span class="enscript-keyword">else</span>
		LocalDomain = <span class="enscript-string">&quot;&quot;</span>;
	(<span class="enscript-type">void</span>)signal(SIGTERM, die);
	(<span class="enscript-type">void</span>)signal(SIGINT, Debug ? die : SIG_IGN);
	(<span class="enscript-type">void</span>)signal(SIGQUIT, Debug ? die : SIG_IGN);
	(<span class="enscript-type">void</span>)signal(SIGCHLD, reapchild);
	(<span class="enscript-type">void</span>)signal(SIGALRM, domark);
	(<span class="enscript-type">void</span>)alarm(TIMERINTVL);
	(<span class="enscript-type">void</span>)unlink(LogName);

#<span class="enscript-reference">ifndef</span> <span class="enscript-variable-name">SUN_LEN</span>
#<span class="enscript-reference">define</span> <span class="enscript-function-name">SUN_LEN</span>(unp) (strlen((unp)-&gt;sun_path) + 2)
#<span class="enscript-reference">endif</span>
	memset(&amp;sunx, 0, <span class="enscript-keyword">sizeof</span>(sunx));
	sunx.sun_family = AF_UNIX;
	(<span class="enscript-type">void</span>)strncpy(sunx.sun_path, LogName, <span class="enscript-keyword">sizeof</span>(sunx.sun_path));
	funix = socket(AF_UNIX, SOCK_DGRAM, 0);
	<span class="enscript-keyword">if</span> (funix &lt; 0 ||
	    bind(funix, (<span class="enscript-type">struct</span> sockaddr *)&amp;sunx, SUN_LEN(&amp;sunx)) &lt; 0 ||
	    chmod(LogName, 0666) &lt; 0) {
		(<span class="enscript-type">void</span>) snprintf(line, <span class="enscript-keyword">sizeof</span> line, <span class="enscript-string">&quot;cannot create %s&quot;</span>, LogName);
		logerror(line);
		dprintf(<span class="enscript-string">&quot;cannot create %s (%d)\n&quot;</span>, LogName, errno);
		die(0);
	}
	finet = socket(AF_INET, SOCK_DGRAM, 0);
	inetm = 0;
	<span class="enscript-keyword">if</span> (finet &gt;= 0) {
		<span class="enscript-type">struct</span> servent *sp;

		sp = getservbyname(<span class="enscript-string">&quot;syslog&quot;</span>, <span class="enscript-string">&quot;udp&quot;</span>);
		<span class="enscript-keyword">if</span> (sp == NULL) {
			errno = 0;
			logerror(<span class="enscript-string">&quot;syslog/udp: unknown service&quot;</span>);
			die(0);
		}
		memset(&amp;sin, 0, <span class="enscript-keyword">sizeof</span>(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = LogPort = sp-&gt;s_port;
		<span class="enscript-keyword">if</span> (bind(finet, (<span class="enscript-type">struct</span> sockaddr *)&amp;sin, <span class="enscript-keyword">sizeof</span>(sin)) &lt; 0) {
			logerror(<span class="enscript-string">&quot;bind&quot;</span>);
			<span class="enscript-keyword">if</span> (!Debug)
				die(0);
		} <span class="enscript-keyword">else</span> {
			inetm = FDMASK(finet);
			InetInuse = 1;
		}
	}
	<span class="enscript-keyword">if</span> ((fklog = open(_PATH_KLOG, O_RDONLY, 0)) &gt;= 0)
		klogm = FDMASK(fklog);
	<span class="enscript-keyword">else</span> {
		dprintf(<span class="enscript-string">&quot;can't open %s (%d)\n&quot;</span>, _PATH_KLOG, errno);
		klogm = 0;
	}

	<span class="enscript-comment">/* tuck my process id away */</span>
	fp = fopen(PidFile, <span class="enscript-string">&quot;w&quot;</span>);
	<span class="enscript-keyword">if</span> (fp != NULL) {
		fprintf(fp, <span class="enscript-string">&quot;%d\n&quot;</span>, getpid());
		(<span class="enscript-type">void</span>) fclose(fp);
	}

	dprintf(<span class="enscript-string">&quot;off &amp; running....\n&quot;</span>);

	init(0);
	(<span class="enscript-type">void</span>)signal(SIGHUP, init);

	<span class="enscript-keyword">for</span> (;;) {
		<span class="enscript-type">int</span> nfds, readfds = FDMASK(funix) | inetm | klogm;

		dprintf(<span class="enscript-string">&quot;readfds = %#x\n&quot;</span>, readfds);
		nfds = select(20, (fd_set *)&amp;readfds, (fd_set *)NULL,
		    (fd_set *)NULL, (<span class="enscript-type">struct</span> timeval *)NULL);
		<span class="enscript-keyword">if</span> (nfds == 0)
			<span class="enscript-keyword">continue</span>;
		<span class="enscript-keyword">if</span> (nfds &lt; 0) {
			<span class="enscript-keyword">if</span> (errno != EINTR)
				logerror(<span class="enscript-string">&quot;select&quot;</span>);
			<span class="enscript-keyword">continue</span>;
		}
		dprintf(<span class="enscript-string">&quot;got a message (%d, %#x)\n&quot;</span>, nfds, readfds);
		<span class="enscript-keyword">if</span> (readfds &amp; klogm) {
			i = read(fklog, line, <span class="enscript-keyword">sizeof</span>(line) - 1);
			<span class="enscript-keyword">if</span> (i &gt; 0) {
				line[i] = <span class="enscript-string">'\0'</span>;
				printsys(line);
			} <span class="enscript-keyword">else</span> <span class="enscript-keyword">if</span> (i &lt; 0 &amp;&amp; errno != EINTR) {
				logerror(<span class="enscript-string">&quot;klog&quot;</span>);
				fklog = -1;
				klogm = 0;
			}
		}
		<span class="enscript-keyword">if</span> (readfds &amp; FDMASK(funix)) {
			len = <span class="enscript-keyword">sizeof</span>(fromunix);
			i = recvfrom(funix, line, MAXLINE, 0,
			    (<span class="enscript-type">struct</span> sockaddr *)&amp;fromunix, &amp;len);
			<span class="enscript-keyword">if</span> (i &gt; 0) {
				line[i] = <span class="enscript-string">'\0'</span>;
				printline(LocalHostName, line);
			} <span class="enscript-keyword">else</span> <span class="enscript-keyword">if</span> (i &lt; 0 &amp;&amp; errno != EINTR)
				logerror(<span class="enscript-string">&quot;recvfrom unix&quot;</span>);
		}
		<span class="enscript-keyword">if</span> (readfds &amp; inetm) {
			len = <span class="enscript-keyword">sizeof</span>(frominet);
			i = recvfrom(finet, line, MAXLINE, 0,
			    (<span class="enscript-type">struct</span> sockaddr *)&amp;frominet, &amp;len);
			<span class="enscript-keyword">if</span> (Insecure) {
				<span class="enscript-keyword">if</span> (i &gt; 0) {
					line[i] = <span class="enscript-string">'\0'</span>;
					printline(cvthname(&amp;frominet), line);
				} <span class="enscript-keyword">else</span> <span class="enscript-keyword">if</span> (i &lt; 0 &amp;&amp; errno != EINTR)
					logerror(<span class="enscript-string">&quot;recvfrom inet&quot;</span>);
			}
		} 
	}
}

<span class="enscript-type">void</span>
<span class="enscript-function-name">usage</span>()
{

	(<span class="enscript-type">void</span>)fprintf(stderr,
	    <span class="enscript-string">&quot;usage: syslogd [-f conffile] [-m markinterval] [-p logpath]\n&quot;</span>);
	exit(1);
}

<span class="enscript-comment">/*
 * Take a raw input line, decode the message, and print the message
 * on the appropriate log files.
 */</span>
<span class="enscript-type">void</span>
<span class="enscript-function-name">printline</span>(hname, msg)
	<span class="enscript-type">char</span> *hname;
	<span class="enscript-type">char</span> *msg;
{
	<span class="enscript-type">int</span> c, pri;
	<span class="enscript-type">char</span> *p, *q, line[MAXLINE + 1];

	<span class="enscript-comment">/* test for special codes */</span>
	pri = DEFUPRI;
	p = msg;
	<span class="enscript-keyword">if</span> (*p == <span class="enscript-string">'&lt;'</span>) {
		pri = 0;
		<span class="enscript-keyword">while</span> (isdigit(*++p))
			pri = 10 * pri + (*p - <span class="enscript-string">'0'</span>);
		<span class="enscript-keyword">if</span> (*p == <span class="enscript-string">'&gt;'</span>)
			++p;
	}
	<span class="enscript-keyword">if</span> (pri &amp;~ (LOG_FACMASK|LOG_PRIMASK))
		pri = DEFUPRI;

	<span class="enscript-comment">/* don't allow users to log kernel messages */</span>
	<span class="enscript-keyword">if</span> (LOG_FAC(pri) == LOG_KERN)
		pri = LOG_MAKEPRI(LOG_USER, LOG_PRI(pri));

	q = line;

	<span class="enscript-keyword">while</span> ((c = *p++) != <span class="enscript-string">'\0'</span> &amp;&amp;
	    q &lt; &amp;line[<span class="enscript-keyword">sizeof</span>(line) - 2]) {
		c &amp;= 0177;
		<span class="enscript-keyword">if</span> (iscntrl(c))
			<span class="enscript-keyword">if</span> (c == <span class="enscript-string">'\n'</span>)
				*q++ = <span class="enscript-string">' '</span>;
			<span class="enscript-keyword">else</span> <span class="enscript-keyword">if</span> (c == <span class="enscript-string">'\t'</span>)
				*q++ = <span class="enscript-string">'\t'</span>;
			<span class="enscript-keyword">else</span> {
				*q++ = <span class="enscript-string">'^'</span>;
				*q++ = c ^ 0100;
			}
		<span class="enscript-keyword">else</span>
			*q++ = c;
	}
	*q = <span class="enscript-string">'\0'</span>;

	logmsg(pri, line, hname, 0);
}

<span class="enscript-comment">/*
 * Take a raw input line from /dev/klog, split and format similar to syslog().
 */</span>
<span class="enscript-type">void</span>
<span class="enscript-function-name">printsys</span>(msg)
	<span class="enscript-type">char</span> *msg;
{
	<span class="enscript-type">int</span> c, pri, flags;
	<span class="enscript-type">char</span> *lp, *p, *q, line[MAXLINE + 1];

	(<span class="enscript-type">void</span>)strcpy(line, <span class="enscript-string">&quot;mach_kernel: &quot;</span>);
	lp = line + strlen(line);
	<span class="enscript-keyword">for</span> (p = msg; *p != <span class="enscript-string">'\0'</span>; ) {
		flags = SYNC_FILE | ADDDATE;	<span class="enscript-comment">/* fsync file after write */</span>
		pri = DEFSPRI;
		<span class="enscript-keyword">if</span> (*p == <span class="enscript-string">'&lt;'</span>) {
			pri = 0;
			<span class="enscript-keyword">while</span> (isdigit(*++p))
				pri = 10 * pri + (*p - <span class="enscript-string">'0'</span>);
			<span class="enscript-keyword">if</span> (*p == <span class="enscript-string">'&gt;'</span>)
				++p;
		} <span class="enscript-keyword">else</span> {
			<span class="enscript-comment">/* kernel printf's come out on console */</span>
			flags |= IGN_CONS;
		}
		<span class="enscript-keyword">if</span> (pri &amp;~ (LOG_FACMASK|LOG_PRIMASK))
			pri = DEFSPRI;
		q = lp;
		<span class="enscript-keyword">while</span> (*p != <span class="enscript-string">'\0'</span> &amp;&amp; (c = *p++) != <span class="enscript-string">'\n'</span> &amp;&amp;
		    q &lt; &amp;line[MAXLINE])
			*q++ = c;
		*q = <span class="enscript-string">'\0'</span>;
		logmsg(pri, line, LocalHostName, flags);
	}
}

time_t	now;

<span class="enscript-comment">/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */</span>
<span class="enscript-type">void</span>
<span class="enscript-function-name">logmsg</span>(pri, msg, from, flags)
	<span class="enscript-type">int</span> pri;
	<span class="enscript-type">char</span> *msg, *from;
	<span class="enscript-type">int</span> flags;
{
	<span class="enscript-type">struct</span> filed *f;
	<span class="enscript-type">int</span> fac, msglen, omask, prilev;
	<span class="enscript-type">char</span> *timestamp;

	dprintf(<span class="enscript-string">&quot;logmsg: pri %o, flags %x, from %s, msg %s\n&quot;</span>,
	    pri, flags, from, msg);

	omask = sigblock(sigmask(SIGHUP)|sigmask(SIGALRM));

	<span class="enscript-comment">/*
	 * Check to see if msg looks non-standard.
	 */</span>
	msglen = strlen(msg);
	<span class="enscript-keyword">if</span> (msglen &lt; 16 || msg[3] != <span class="enscript-string">' '</span> || msg[6] != <span class="enscript-string">' '</span> ||
	    msg[9] != <span class="enscript-string">':'</span> || msg[12] != <span class="enscript-string">':'</span> || msg[15] != <span class="enscript-string">' '</span>)
		flags |= ADDDATE;

	(<span class="enscript-type">void</span>)time(&amp;now);
	<span class="enscript-keyword">if</span> (flags &amp; ADDDATE)
		timestamp = ctime(&amp;now) + 4;
	<span class="enscript-keyword">else</span> {
		timestamp = msg;
		msg += 16;
		msglen -= 16;
	}

	<span class="enscript-comment">/* extract facility and priority level */</span>
	<span class="enscript-keyword">if</span> (flags &amp; MARK)
		fac = LOG_NFACILITIES;
	<span class="enscript-keyword">else</span>
		fac = LOG_FAC(pri);
	prilev = LOG_PRI(pri);

	<span class="enscript-comment">/* log the message to the particular outputs */</span>
	<span class="enscript-keyword">if</span> (!Initialized) {
		f = &amp;consfile;
		f-&gt;f_file = open(ctty, O_WRONLY, 0);

		<span class="enscript-keyword">if</span> (f-&gt;f_file &gt;= 0) {
			fprintlog(f, flags, msg);
			(<span class="enscript-type">void</span>)close(f-&gt;f_file);
		}
		(<span class="enscript-type">void</span>)sigsetmask(omask);
		<span class="enscript-keyword">return</span>;
	}
	<span class="enscript-keyword">for</span> (f = Files; f; f = f-&gt;f_next) {
		<span class="enscript-comment">/* skip messages that are incorrect priority */</span>
		<span class="enscript-keyword">if</span> (f-&gt;f_pmask[fac] &lt; prilev ||
		    f-&gt;f_pmask[fac] == INTERNAL_NOPRI)
			<span class="enscript-keyword">continue</span>;

		<span class="enscript-keyword">if</span> (f-&gt;f_type == F_CONSOLE &amp;&amp; (flags &amp; IGN_CONS))
			<span class="enscript-keyword">continue</span>;

		<span class="enscript-comment">/* don't output marks to recently written files */</span>
		<span class="enscript-keyword">if</span> ((flags &amp; MARK) &amp;&amp; (now - f-&gt;f_time) &lt; MarkInterval / 2)
			<span class="enscript-keyword">continue</span>;

		<span class="enscript-comment">/*
		 * suppress duplicate lines to this file
		 */</span>
		<span class="enscript-keyword">if</span> ((flags &amp; MARK) == 0 &amp;&amp; msglen == f-&gt;f_prevlen &amp;&amp;
		    !strcmp(msg, f-&gt;f_prevline) &amp;&amp;
		    !strcmp(from, f-&gt;f_prevhost)) {
			(<span class="enscript-type">void</span>)strncpy(f-&gt;f_lasttime, timestamp, 15);
			f-&gt;f_prevcount++;
			dprintf(<span class="enscript-string">&quot;msg repeated %d times, %ld sec of %d\n&quot;</span>,
			    f-&gt;f_prevcount, now - f-&gt;f_time,
			    repeatinterval[f-&gt;f_repeatcount]);
			<span class="enscript-comment">/*
			 * If domark would have logged this by now,
			 * flush it now (so we don't hold isolated messages),
			 * but back off so we'll flush less often
			 * in the future.
			 */</span>
			<span class="enscript-keyword">if</span> (now &gt; REPEATTIME(f)) {
				fprintlog(f, flags, (<span class="enscript-type">char</span> *)NULL);
				BACKOFF(f);
			}
		} <span class="enscript-keyword">else</span> {
			<span class="enscript-comment">/* new line, save it */</span>
			<span class="enscript-keyword">if</span> (f-&gt;f_prevcount)
				fprintlog(f, 0, (<span class="enscript-type">char</span> *)NULL);
			f-&gt;f_repeatcount = 0;
			(<span class="enscript-type">void</span>)strncpy(f-&gt;f_lasttime, timestamp, 15);
			(<span class="enscript-type">void</span>)strncpy(f-&gt;f_prevhost, from,
					<span class="enscript-keyword">sizeof</span>(f-&gt;f_prevhost));
			<span class="enscript-keyword">if</span> (msglen &lt; MAXSVLINE) {
				f-&gt;f_prevlen = msglen;
				f-&gt;f_prevpri = pri;
				(<span class="enscript-type">void</span>)strcpy(f-&gt;f_prevline, msg);
				fprintlog(f, flags, (<span class="enscript-type">char</span> *)NULL);
			} <span class="enscript-keyword">else</span> {
				f-&gt;f_prevline[0] = 0;
				f-&gt;f_prevlen = 0;
				fprintlog(f, flags, msg);
			}
		}
	}
	(<span class="enscript-type">void</span>)sigsetmask(omask);
}

<span class="enscript-type">void</span>
<span class="enscript-function-name">fprintlog</span>(f, flags, msg)
	<span class="enscript-type">struct</span> filed *f;
	<span class="enscript-type">int</span> flags;
	<span class="enscript-type">char</span> *msg;
{
	<span class="enscript-type">struct</span> iovec iov[6];
	<span class="enscript-type">struct</span> iovec *v;
	<span class="enscript-type">int</span> l;
	<span class="enscript-type">char</span> line[MAXLINE + 1], repbuf[80], greetings[200];

	v = iov;
	<span class="enscript-keyword">if</span> (f-&gt;f_type == F_WALL) {
		v-&gt;iov_base = greetings;
		v-&gt;iov_len = snprintf(greetings, <span class="enscript-keyword">sizeof</span> greetings, 
		    <span class="enscript-string">&quot;\r\n\7Message from syslogd@%s at %.24s ...\r\n&quot;</span>,
		    f-&gt;f_prevhost, ctime(&amp;now));
		v++;
		v-&gt;iov_base = <span class="enscript-string">&quot;&quot;</span>;
		v-&gt;iov_len = 0;
		v++;
	} <span class="enscript-keyword">else</span> {
		v-&gt;iov_base = f-&gt;f_lasttime;
		v-&gt;iov_len = 15;
		v++;
		v-&gt;iov_base = <span class="enscript-string">&quot; &quot;</span>;
		v-&gt;iov_len = 1;
		v++;
	}
	v-&gt;iov_base = f-&gt;f_prevhost;
	v-&gt;iov_len = strlen(v-&gt;iov_base);
	v++;
	v-&gt;iov_base = <span class="enscript-string">&quot; &quot;</span>;
	v-&gt;iov_len = 1;
	v++;

	<span class="enscript-keyword">if</span> (msg) {
		v-&gt;iov_base = msg;
		v-&gt;iov_len = strlen(msg);
	} <span class="enscript-keyword">else</span> <span class="enscript-keyword">if</span> (f-&gt;f_prevcount &gt; 1) {
		v-&gt;iov_base = repbuf;
		v-&gt;iov_len = snprintf(repbuf, <span class="enscript-keyword">sizeof</span> repbuf, <span class="enscript-string">&quot;last message repeated %d times&quot;</span>,
		    f-&gt;f_prevcount);
	} <span class="enscript-keyword">else</span> {
		v-&gt;iov_base = f-&gt;f_prevline;
		v-&gt;iov_len = f-&gt;f_prevlen;
	}
	v++;

	dprintf(<span class="enscript-string">&quot;Logging to %s&quot;</span>, TypeNames[f-&gt;f_type]);
	f-&gt;f_time = now;

	<span class="enscript-keyword">switch</span> (f-&gt;f_type) {
	<span class="enscript-keyword">case</span> <span class="enscript-reference">F_UNUSED</span>:
		dprintf(<span class="enscript-string">&quot;\n&quot;</span>);
		<span class="enscript-keyword">break</span>;

	<span class="enscript-keyword">case</span> <span class="enscript-reference">F_FORW</span>:
		dprintf(<span class="enscript-string">&quot; %s\n&quot;</span>, f-&gt;f_un.f_forw.f_hname);
		l = snprintf(line, <span class="enscript-keyword">sizeof</span> line, <span class="enscript-string">&quot;&lt;%d&gt;%.15s %s&quot;</span>, f-&gt;f_prevpri,
		    iov[0].iov_base, iov[4].iov_base);
		<span class="enscript-keyword">if</span> (l &gt; MAXLINE)
			l = MAXLINE;
		<span class="enscript-keyword">if</span> (sendto(finet, line, l, 0,
		    (<span class="enscript-type">struct</span> sockaddr *)&amp;f-&gt;f_un.f_forw.f_addr,
		    <span class="enscript-keyword">sizeof</span>(f-&gt;f_un.f_forw.f_addr)) != l) {
			<span class="enscript-type">int</span> e = errno;
			(<span class="enscript-type">void</span>)close(f-&gt;f_file);
			f-&gt;f_type = F_UNUSED;
			errno = e;
			logerror(<span class="enscript-string">&quot;sendto&quot;</span>);
		}
		<span class="enscript-keyword">break</span>;

	<span class="enscript-keyword">case</span> <span class="enscript-reference">F_CONSOLE</span>:
		<span class="enscript-keyword">if</span> (flags &amp; IGN_CONS) {
			dprintf(<span class="enscript-string">&quot; (ignored)\n&quot;</span>);
			<span class="enscript-keyword">break</span>;
		}
		<span class="enscript-comment">/* FALLTHROUGH */</span>

	<span class="enscript-keyword">case</span> <span class="enscript-reference">F_TTY</span>:
	<span class="enscript-keyword">case</span> <span class="enscript-reference">F_FILE</span>:
		dprintf(<span class="enscript-string">&quot; %s\n&quot;</span>, f-&gt;f_un.f_fname);
		<span class="enscript-keyword">if</span> (f-&gt;f_type != F_FILE) {
			v-&gt;iov_base = <span class="enscript-string">&quot;\r\n&quot;</span>;
			v-&gt;iov_len = 2;
		} <span class="enscript-keyword">else</span> {
			v-&gt;iov_base = <span class="enscript-string">&quot;\n&quot;</span>;
			v-&gt;iov_len = 1;
		}
	<span class="enscript-reference">again</span>:
		<span class="enscript-keyword">if</span> (writev(f-&gt;f_file, iov, 6) &lt; 0) {
			<span class="enscript-type">int</span> e = errno;
			(<span class="enscript-type">void</span>)close(f-&gt;f_file);
			<span class="enscript-comment">/*
			 * Check for errors on TTY's due to loss of tty
			 */</span>
			<span class="enscript-keyword">if</span> ((e == EIO || e == EBADF) &amp;&amp; f-&gt;f_type != F_FILE) {
				f-&gt;f_file = open(f-&gt;f_un.f_fname,
				    O_WRONLY|O_APPEND, 0);
				<span class="enscript-keyword">if</span> (f-&gt;f_file &lt; 0) {
					f-&gt;f_type = F_UNUSED;
					logerror(f-&gt;f_un.f_fname);
				} <span class="enscript-keyword">else</span>
					<span class="enscript-keyword">goto</span> <span class="enscript-reference">again</span>;
			} <span class="enscript-keyword">else</span> {
				f-&gt;f_type = F_UNUSED;
				errno = e;
				logerror(f-&gt;f_un.f_fname);
			}
		} <span class="enscript-keyword">else</span> <span class="enscript-keyword">if</span> (flags &amp; SYNC_FILE)
			(<span class="enscript-type">void</span>)fsync(f-&gt;f_file);
		<span class="enscript-keyword">break</span>;

	<span class="enscript-keyword">case</span> <span class="enscript-reference">F_USERS</span>:
	<span class="enscript-keyword">case</span> <span class="enscript-reference">F_WALL</span>:
		dprintf(<span class="enscript-string">&quot;\n&quot;</span>);
		v-&gt;iov_base = <span class="enscript-string">&quot;\r\n&quot;</span>;
		v-&gt;iov_len = 2;
		wallmsg(f, iov);
		<span class="enscript-keyword">break</span>;
	}
	f-&gt;f_prevcount = 0;
}

<span class="enscript-comment">/*
 *  WALLMSG -- Write a message to the world at large
 *
 *	Write the specified message to either the entire
 *	world, or a list of approved users.
 */</span>
<span class="enscript-type">void</span>
<span class="enscript-function-name">wallmsg</span>(f, iov)
	<span class="enscript-type">struct</span> filed *f;
	<span class="enscript-type">struct</span> iovec *iov;
{
	<span class="enscript-type">static</span> <span class="enscript-type">int</span> reenter;			<span class="enscript-comment">/* avoid calling ourselves */</span>
	FILE *uf;
	<span class="enscript-type">struct</span> utmp ut;
	<span class="enscript-type">int</span> i;
	<span class="enscript-type">char</span> *p;
	<span class="enscript-type">char</span> line[<span class="enscript-keyword">sizeof</span>(ut.ut_line) + 1];

	<span class="enscript-keyword">if</span> (reenter++)
		<span class="enscript-keyword">return</span>;
	<span class="enscript-keyword">if</span> ((uf = fopen(_PATH_UTMP, <span class="enscript-string">&quot;r&quot;</span>)) == NULL) {
		logerror(_PATH_UTMP);
		reenter = 0;
		<span class="enscript-keyword">return</span>;
	}
	<span class="enscript-comment">/* NOSTRICT */</span>
	<span class="enscript-keyword">while</span> (fread((<span class="enscript-type">char</span> *)&amp;ut, <span class="enscript-keyword">sizeof</span>(ut), 1, uf) == 1) {
		<span class="enscript-keyword">if</span> (ut.ut_name[0] == <span class="enscript-string">'\0'</span>)
			<span class="enscript-keyword">continue</span>;
		strncpy(line, ut.ut_line, <span class="enscript-keyword">sizeof</span>(ut.ut_line));
		line[<span class="enscript-keyword">sizeof</span>(ut.ut_line)] = <span class="enscript-string">'\0'</span>;
		<span class="enscript-keyword">if</span> (f-&gt;f_type == F_WALL) {
			<span class="enscript-keyword">if</span> ((p = ttymsg(iov, 6, line, 60*5)) != NULL) {
				errno = 0;	<span class="enscript-comment">/* already in msg */</span>
				logerror(p);
			}
			<span class="enscript-keyword">continue</span>;
		}
		<span class="enscript-comment">/* should we send the message to this user? */</span>
		<span class="enscript-keyword">for</span> (i = 0; i &lt; MAXUNAMES; i++) {
			<span class="enscript-keyword">if</span> (!f-&gt;f_un.f_uname[i][0])
				<span class="enscript-keyword">break</span>;
			<span class="enscript-keyword">if</span> (!strncmp(f-&gt;f_un.f_uname[i], ut.ut_name,
			    UT_NAMESIZE)) {
				<span class="enscript-keyword">if</span> ((p = ttymsg(iov, 6, line, 60*5)) != NULL) {
					errno = 0;	<span class="enscript-comment">/* already in msg */</span>
					logerror(p);
				}
				<span class="enscript-keyword">break</span>;
			}
		}
	}
	(<span class="enscript-type">void</span>)fclose(uf);
	reenter = 0;
}

<span class="enscript-type">void</span>
<span class="enscript-function-name">reapchild</span>(signo)
	<span class="enscript-type">int</span> signo;
{
	<span class="enscript-type">union</span> wait status;

	<span class="enscript-keyword">while</span> (wait3((<span class="enscript-type">int</span> *)&amp;status, WNOHANG, (<span class="enscript-type">struct</span> rusage *)NULL) &gt; 0)
		;
}

<span class="enscript-comment">/*
 * Return a printable representation of a host address.
 */</span>
<span class="enscript-type">char</span> *
<span class="enscript-function-name">cvthname</span>(f)
	<span class="enscript-type">struct</span> sockaddr_in *f;
{
	<span class="enscript-type">struct</span> hostent *hp;
	<span class="enscript-type">char</span> *p;

	dprintf(<span class="enscript-string">&quot;cvthname(%s)\n&quot;</span>, inet_ntoa(f-&gt;sin_addr));

	<span class="enscript-keyword">if</span> (f-&gt;sin_family != AF_INET) {
		dprintf(<span class="enscript-string">&quot;Malformed from address\n&quot;</span>);
		<span class="enscript-keyword">return</span> (<span class="enscript-string">&quot;???&quot;</span>);
	}
	hp = gethostbyaddr((<span class="enscript-type">char</span> *)&amp;f-&gt;sin_addr,
	    <span class="enscript-keyword">sizeof</span>(<span class="enscript-type">struct</span> in_addr), f-&gt;sin_family);
	<span class="enscript-keyword">if</span> (hp == 0) {
		dprintf(<span class="enscript-string">&quot;Host name for your address (%s) unknown\n&quot;</span>,
			inet_ntoa(f-&gt;sin_addr));
		<span class="enscript-keyword">return</span> (inet_ntoa(f-&gt;sin_addr));
	}
	<span class="enscript-keyword">if</span> ((p = strchr(hp-&gt;h_name, <span class="enscript-string">'.'</span>)) &amp;&amp; strcmp(p + 1, LocalDomain) == 0)
		*p = <span class="enscript-string">'\0'</span>;
	<span class="enscript-keyword">return</span> (hp-&gt;h_name);
}

<span class="enscript-type">void</span>
<span class="enscript-function-name">domark</span>(signo)
	<span class="enscript-type">int</span> signo;
{
	<span class="enscript-type">struct</span> filed *f;

	now = time((time_t *)NULL);
	MarkSeq += TIMERINTVL;
	<span class="enscript-keyword">if</span> (MarkSeq &gt;= MarkInterval) {
		logmsg(LOG_INFO, <span class="enscript-string">&quot;-- MARK --&quot;</span>, LocalHostName, ADDDATE|MARK);
		MarkSeq = 0;
	}

	<span class="enscript-keyword">for</span> (f = Files; f; f = f-&gt;f_next) {
		<span class="enscript-keyword">if</span> (f-&gt;f_prevcount &amp;&amp; now &gt;= REPEATTIME(f)) {
			dprintf(<span class="enscript-string">&quot;flush %s: repeated %d times, %d sec.\n&quot;</span>,
			    TypeNames[f-&gt;f_type], f-&gt;f_prevcount,
			    repeatinterval[f-&gt;f_repeatcount]);
			fprintlog(f, 0, (<span class="enscript-type">char</span> *)NULL);
			BACKOFF(f);
		}
	}
	(<span class="enscript-type">void</span>)alarm(TIMERINTVL);
}

<span class="enscript-comment">/*
 * Print syslogd errors some place.
 */</span>
<span class="enscript-type">void</span>
<span class="enscript-function-name">logerror</span>(type)
	<span class="enscript-type">char</span> *type;
{
	<span class="enscript-type">char</span> buf[100];

	<span class="enscript-keyword">if</span> (errno)
		(<span class="enscript-type">void</span>)snprintf(buf,
		    <span class="enscript-keyword">sizeof</span>(buf), <span class="enscript-string">&quot;syslogd: %s: %s&quot;</span>, type, strerror(errno));
	<span class="enscript-keyword">else</span>
		(<span class="enscript-type">void</span>)snprintf(buf, <span class="enscript-keyword">sizeof</span>(buf), <span class="enscript-string">&quot;syslogd: %s&quot;</span>, type);
	errno = 0;
	dprintf(<span class="enscript-string">&quot;%s\n&quot;</span>, buf);
	logmsg(LOG_SYSLOG|LOG_ERR, buf, LocalHostName, ADDDATE);
}

<span class="enscript-type">void</span>
<span class="enscript-function-name">die</span>(signo)
	<span class="enscript-type">int</span> signo;
{
	<span class="enscript-type">struct</span> filed *f;
	<span class="enscript-type">char</span> buf[100];

	<span class="enscript-keyword">for</span> (f = Files; f != NULL; f = f-&gt;f_next) {
		<span class="enscript-comment">/* flush any pending output */</span>
		<span class="enscript-keyword">if</span> (f-&gt;f_prevcount)
			fprintlog(f, 0, (<span class="enscript-type">char</span> *)NULL);
	}
	<span class="enscript-keyword">if</span> (signo) {
		dprintf(<span class="enscript-string">&quot;syslogd: exiting on signal %d\n&quot;</span>, signo);
		(<span class="enscript-type">void</span>)snprintf(buf, <span class="enscript-keyword">sizeof</span> buf, <span class="enscript-string">&quot;exiting on signal %d&quot;</span>, signo);
		errno = 0;
		logerror(buf);
	}
	(<span class="enscript-type">void</span>)unlink(LogName);
	exit(0);
}

<span class="enscript-comment">/*
 *  INIT -- Initialize syslogd from configuration table
 */</span>
<span class="enscript-type">void</span>
<span class="enscript-function-name">init</span>(signo)
	<span class="enscript-type">int</span> signo;
{
	<span class="enscript-type">int</span> i;
	FILE *cf;
	<span class="enscript-type">struct</span> filed *f, *next, **nextp;
	<span class="enscript-type">char</span> *p;
	<span class="enscript-type">char</span> cline[LINE_MAX];

	dprintf(<span class="enscript-string">&quot;init\n&quot;</span>);

	<span class="enscript-comment">/*
	 *  Close all open log files.
	 */</span>
	Initialized = 0;
	<span class="enscript-keyword">for</span> (f = Files; f != NULL; f = next) {
		<span class="enscript-comment">/* flush any pending output */</span>
		<span class="enscript-keyword">if</span> (f-&gt;f_prevcount)
			fprintlog(f, 0, (<span class="enscript-type">char</span> *)NULL);

		<span class="enscript-keyword">switch</span> (f-&gt;f_type) {
		<span class="enscript-keyword">case</span> <span class="enscript-reference">F_FILE</span>:
		<span class="enscript-keyword">case</span> <span class="enscript-reference">F_TTY</span>:
		<span class="enscript-keyword">case</span> <span class="enscript-reference">F_CONSOLE</span>:
		<span class="enscript-keyword">case</span> <span class="enscript-reference">F_FORW</span>:
			(<span class="enscript-type">void</span>)close(f-&gt;f_file);
			<span class="enscript-keyword">break</span>;
		}
		next = f-&gt;f_next;
		free((<span class="enscript-type">char</span> *)f);
	}
	Files = NULL;
	nextp = &amp;Files;

	<span class="enscript-comment">/* open the configuration file */</span>
	<span class="enscript-keyword">if</span> ((cf = fopen(ConfFile, <span class="enscript-string">&quot;r&quot;</span>)) == NULL) {
		dprintf(<span class="enscript-string">&quot;cannot open %s\n&quot;</span>, ConfFile);
		*nextp = (<span class="enscript-type">struct</span> filed *)calloc(1, <span class="enscript-keyword">sizeof</span>(*f));
		cfline(<span class="enscript-string">&quot;*.ERR\t/dev/console&quot;</span>, *nextp);
		(*nextp)-&gt;f_next = (<span class="enscript-type">struct</span> filed *)calloc(1, <span class="enscript-keyword">sizeof</span>(*f));
		cfline(<span class="enscript-string">&quot;*.PANIC\t*&quot;</span>, (*nextp)-&gt;f_next);
		Initialized = 1;
		<span class="enscript-keyword">return</span>;
	}

	<span class="enscript-comment">/*
	 *  Foreach line in the conf table, open that file.
	 */</span>
	f = NULL;
	<span class="enscript-keyword">while</span> (fgets(cline, <span class="enscript-keyword">sizeof</span>(cline), cf) != NULL) {
		<span class="enscript-comment">/*
		 * check for end-of-section, comments, strip off trailing
		 * spaces and newline character.
		 */</span>
		<span class="enscript-keyword">for</span> (p = cline; isspace(*p); ++p)
			<span class="enscript-keyword">continue</span>;
		<span class="enscript-keyword">if</span> (*p == NULL || *p == <span class="enscript-string">'#'</span>)
			<span class="enscript-keyword">continue</span>;
		<span class="enscript-keyword">for</span> (p = strchr(cline, <span class="enscript-string">'\0'</span>); isspace(*--p);)
			<span class="enscript-keyword">continue</span>;
		*++p = <span class="enscript-string">'\0'</span>;
		f = (<span class="enscript-type">struct</span> filed *)calloc(1, <span class="enscript-keyword">sizeof</span>(*f));
		*nextp = f;
		nextp = &amp;f-&gt;f_next;
		cfline(cline, f);
	}

	<span class="enscript-comment">/* close the configuration file */</span>
	(<span class="enscript-type">void</span>)fclose(cf);

	Initialized = 1;

	<span class="enscript-keyword">if</span> (Debug) {
		<span class="enscript-keyword">for</span> (f = Files; f; f = f-&gt;f_next) {
			<span class="enscript-keyword">for</span> (i = 0; i &lt;= LOG_NFACILITIES; i++)
				<span class="enscript-keyword">if</span> (f-&gt;f_pmask[i] == INTERNAL_NOPRI)
					printf(<span class="enscript-string">&quot;X &quot;</span>);
				<span class="enscript-keyword">else</span>
					printf(<span class="enscript-string">&quot;%d &quot;</span>, f-&gt;f_pmask[i]);
			printf(<span class="enscript-string">&quot;%s: &quot;</span>, TypeNames[f-&gt;f_type]);
			<span class="enscript-keyword">switch</span> (f-&gt;f_type) {
			<span class="enscript-keyword">case</span> <span class="enscript-reference">F_FILE</span>:
			<span class="enscript-keyword">case</span> <span class="enscript-reference">F_TTY</span>:
			<span class="enscript-keyword">case</span> <span class="enscript-reference">F_CONSOLE</span>:
				printf(<span class="enscript-string">&quot;%s&quot;</span>, f-&gt;f_un.f_fname);
				<span class="enscript-keyword">break</span>;

			<span class="enscript-keyword">case</span> <span class="enscript-reference">F_FORW</span>:
				printf(<span class="enscript-string">&quot;%s&quot;</span>, f-&gt;f_un.f_forw.f_hname);
				<span class="enscript-keyword">break</span>;

			<span class="enscript-keyword">case</span> <span class="enscript-reference">F_USERS</span>:
				<span class="enscript-keyword">for</span> (i = 0; i &lt; MAXUNAMES &amp;&amp; *f-&gt;f_un.f_uname[i]; i++)
					printf(<span class="enscript-string">&quot;%s, &quot;</span>, f-&gt;f_un.f_uname[i]);
				<span class="enscript-keyword">break</span>;
			}
			printf(<span class="enscript-string">&quot;\n&quot;</span>);
		}
	}

	logmsg(LOG_SYSLOG|LOG_INFO, <span class="enscript-string">&quot;syslogd: restart&quot;</span>, LocalHostName, ADDDATE);
	dprintf(<span class="enscript-string">&quot;syslogd: restarted\n&quot;</span>);
}

<span class="enscript-comment">/*
 * Crack a configuration file line
 */</span>
<span class="enscript-type">void</span>
<span class="enscript-function-name">cfline</span>(line, f)
	<span class="enscript-type">char</span> *line;
	<span class="enscript-type">struct</span> filed *f;
{
	<span class="enscript-type">struct</span> hostent *hp;
	<span class="enscript-type">int</span> i, pri;
	<span class="enscript-type">char</span> *bp, *p, *q;
	<span class="enscript-type">char</span> buf[MAXLINE], ebuf[100];

	dprintf(<span class="enscript-string">&quot;cfline(%s)\n&quot;</span>, line);

	errno = 0;	<span class="enscript-comment">/* keep strerror() stuff out of logerror messages */</span>

	<span class="enscript-comment">/* clear out file entry */</span>
	memset(f, 0, <span class="enscript-keyword">sizeof</span>(*f));
	<span class="enscript-keyword">for</span> (i = 0; i &lt;= LOG_NFACILITIES; i++)
		f-&gt;f_pmask[i] = INTERNAL_NOPRI;

	<span class="enscript-comment">/* scan through the list of selectors */</span>
	<span class="enscript-keyword">for</span> (p = line; *p &amp;&amp; *p != <span class="enscript-string">'\t'</span>;) {

		<span class="enscript-comment">/* find the end of this facility name list */</span>
		<span class="enscript-keyword">for</span> (q = p; *q &amp;&amp; *q != <span class="enscript-string">'\t'</span> &amp;&amp; *q++ != <span class="enscript-string">'.'</span>; )
			<span class="enscript-keyword">continue</span>;

		<span class="enscript-comment">/* collect priority name */</span>
		<span class="enscript-keyword">for</span> (bp = buf; *q &amp;&amp; !strchr(<span class="enscript-string">&quot;\t,;&quot;</span>, *q); )
			*bp++ = *q++;
		*bp = <span class="enscript-string">'\0'</span>;

		<span class="enscript-comment">/* skip cruft */</span>
		<span class="enscript-keyword">while</span> (strchr(<span class="enscript-string">&quot;, ;&quot;</span>, *q))
			q++;

		<span class="enscript-comment">/* decode priority name */</span>
		<span class="enscript-keyword">if</span> (*buf == <span class="enscript-string">'*'</span>)
			pri = LOG_PRIMASK + 1;
		<span class="enscript-keyword">else</span> {
			pri = decode(buf, prioritynames);
			<span class="enscript-keyword">if</span> (pri &lt; 0) {
				(<span class="enscript-type">void</span>)snprintf(ebuf, <span class="enscript-keyword">sizeof</span> ebuf, 
				    <span class="enscript-string">&quot;unknown priority name \&quot;%s\&quot;&quot;</span>, buf);
				logerror(ebuf);
				<span class="enscript-keyword">return</span>;
			}
		}

		<span class="enscript-comment">/* scan facilities */</span>
		<span class="enscript-keyword">while</span> (*p &amp;&amp; !strchr(<span class="enscript-string">&quot;\t.;&quot;</span>, *p)) {
			<span class="enscript-keyword">for</span> (bp = buf; *p &amp;&amp; !strchr(<span class="enscript-string">&quot;\t,;.&quot;</span>, *p); )
				*bp++ = *p++;
			*bp = <span class="enscript-string">'\0'</span>;
			<span class="enscript-keyword">if</span> (*buf == <span class="enscript-string">'*'</span>)
				<span class="enscript-keyword">for</span> (i = 0; i &lt; LOG_NFACILITIES; i++)
					f-&gt;f_pmask[i] = pri;
			<span class="enscript-keyword">else</span> {
				i = decode(buf, facilitynames);
				<span class="enscript-keyword">if</span> (i &lt; 0) {
					(<span class="enscript-type">void</span>)snprintf(ebuf, <span class="enscript-keyword">sizeof</span> ebuf, 
					    <span class="enscript-string">&quot;unknown facility name \&quot;%s\&quot;&quot;</span>,
					    buf);
					logerror(ebuf);
					<span class="enscript-keyword">return</span>;
				}
				f-&gt;f_pmask[i &gt;&gt; 3] = pri;
			}
			<span class="enscript-keyword">while</span> (*p == <span class="enscript-string">','</span> || *p == <span class="enscript-string">' '</span>)
				p++;
		}

		p = q;
	}

	<span class="enscript-comment">/* skip to action part */</span>
	<span class="enscript-keyword">while</span> (*p == <span class="enscript-string">'\t'</span>)
		p++;

	<span class="enscript-keyword">switch</span> (*p)
	{
	<span class="enscript-keyword">case</span> <span class="enscript-string">'@'</span>:
		<span class="enscript-keyword">if</span> (!InetInuse)
			<span class="enscript-keyword">break</span>;
		(<span class="enscript-type">void</span>)strcpy(f-&gt;f_un.f_forw.f_hname, ++p);
		hp = gethostbyname(p);
		<span class="enscript-keyword">if</span> (hp == NULL) {
			<span class="enscript-type">extern</span> <span class="enscript-type">int</span> h_errno;

			logerror(hstrerror(h_errno));
			<span class="enscript-keyword">break</span>;
		}
		memset(&amp;f-&gt;f_un.f_forw.f_addr, 0,
			 <span class="enscript-keyword">sizeof</span>(f-&gt;f_un.f_forw.f_addr));
		f-&gt;f_un.f_forw.f_addr.sin_family = AF_INET;
		f-&gt;f_un.f_forw.f_addr.sin_port = LogPort;
		memmove(&amp;f-&gt;f_un.f_forw.f_addr.sin_addr, hp-&gt;h_addr, hp-&gt;h_length);
		f-&gt;f_type = F_FORW;
		<span class="enscript-keyword">break</span>;

	<span class="enscript-keyword">case</span> <span class="enscript-string">'/'</span>:
		(<span class="enscript-type">void</span>)strcpy(f-&gt;f_un.f_fname, p);
		<span class="enscript-keyword">if</span> ((f-&gt;f_file = open(p, O_WRONLY|O_APPEND, 0)) &lt; 0) {
			f-&gt;f_file = F_UNUSED;
			logerror(p);
			<span class="enscript-keyword">break</span>;
		}
		<span class="enscript-keyword">if</span> (isatty(f-&gt;f_file))
			f-&gt;f_type = F_TTY;
		<span class="enscript-keyword">else</span>
			f-&gt;f_type = F_FILE;
		<span class="enscript-keyword">if</span> (strcmp(p, ctty) == 0)
			f-&gt;f_type = F_CONSOLE;
		<span class="enscript-keyword">break</span>;

	<span class="enscript-keyword">case</span> <span class="enscript-string">'*'</span>:
		f-&gt;f_type = F_WALL;
		<span class="enscript-keyword">break</span>;

	<span class="enscript-reference">default</span>:
		<span class="enscript-keyword">for</span> (i = 0; i &lt; MAXUNAMES &amp;&amp; *p; i++) {
			<span class="enscript-keyword">for</span> (q = p; *q &amp;&amp; *q != <span class="enscript-string">','</span>; )
				q++;
			(<span class="enscript-type">void</span>)strncpy(f-&gt;f_un.f_uname[i], p, UT_NAMESIZE);
			<span class="enscript-keyword">if</span> ((q - p) &gt; UT_NAMESIZE)
				f-&gt;f_un.f_uname[i][UT_NAMESIZE] = <span class="enscript-string">'\0'</span>;
			<span class="enscript-keyword">else</span>
				f-&gt;f_un.f_uname[i][q - p] = <span class="enscript-string">'\0'</span>;
			<span class="enscript-keyword">while</span> (*q == <span class="enscript-string">','</span> || *q == <span class="enscript-string">' '</span>)
				q++;
			p = q;
		}
		f-&gt;f_type = F_USERS;
		<span class="enscript-keyword">break</span>;
	}
}


<span class="enscript-comment">/*
 *  Decode a symbolic name to a numeric value
 */</span>
<span class="enscript-type">int</span>
<span class="enscript-function-name">decode</span>(name, codetab)
	<span class="enscript-type">const</span> <span class="enscript-type">char</span> *name;
	CODE *codetab;
{
	CODE *c;
	<span class="enscript-type">char</span> *p, buf[40];

	<span class="enscript-keyword">if</span> (isdigit(*name))
		<span class="enscript-keyword">return</span> (atoi(name));

	<span class="enscript-keyword">for</span> (p = buf; *name &amp;&amp; p &lt; &amp;buf[<span class="enscript-keyword">sizeof</span>(buf) - 1]; p++, name++) {
		<span class="enscript-keyword">if</span> (isupper(*name))
			*p = tolower(*name);
		<span class="enscript-keyword">else</span>
			*p = *name;
	}
	*p = <span class="enscript-string">'\0'</span>;
	<span class="enscript-keyword">for</span> (c = codetab; c-&gt;c_name; c++)
		<span class="enscript-keyword">if</span> (!strcmp(buf, c-&gt;c_name))
			<span class="enscript-keyword">return</span> (c-&gt;c_val);

	<span class="enscript-keyword">return</span> (-1);
}
</pre>
<hr />
</body></html>