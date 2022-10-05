/**
 * \file
 *
 * \brief Base Node Manager Main.
 *
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

/************************************************************
*       Includes                                            *
*************************************************************/
#include "vty.h"
#include "vtysh.h"
#include "command.h"
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include "vtysh_config.h"
#include "prime_log.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_manager_vty.h"

/************************************************************
*       Defines                                             *
*************************************************************/
#define PRIME_DEFAULT_CONFIG   "prime.conf"
#define PRIME_DEFAULT_LOGFILE  "prime.log"

/* Help information display. */
static void usage (char *progname, int status)
{
	printf ("Usage : %s [OPTION...]\n"
		"\t-b, --boot          Execute boot startup configuration\n"
		"\t-e, --eval          Execute argument as command\n"
		"\t-c, --config        Load the config file,default /etc/config/prime.conf\n"
		"\t-l, --loglevel      Loglevel <0-3>, default 3 (LOG_INFO)\n"
		"\t-f, --logfile       Logfile, default /tmp/prime.log\n"
		"\t-v, --version       Show the version\n"
		"\t-h, --help          Display this help and exit\n", progname);
	exit (status);
}

/* VTY shell options, we use GNU getopt library. */
struct option longopts[] =
{
	{ "boot",	no_argument,		NULL, 'b'},
	{ "eval",	required_argument,	NULL, 'e'},
	{ "config",	required_argument,	NULL, 'c'},
	{ "loglevel",	required_argument,	NULL, 'l'},
	{ "logfile",	required_argument,	NULL, 'f'},
	{ "version",	no_argument,		NULL, 'v'},
	{ "help",	no_argument,		NULL, 'h'},
	{ 0 }
};

/* Initialization of signal handles. */
#include <readline/readline.h>
#include <readline/history.h>

static void my_sig(int sig)
{
	// ctrl+ c
	rl_replace_line("", 0);
	rl_crlf();
	rl_forced_update_display();
}

static void signal_init ()
{
  signal (SIGINT, my_sig );
  signal (SIGTSTP, SIG_IGN);
  signal (SIGPIPE, SIG_IGN);
}

static void in_show_welcome()
{
	char build[1024];
	FILE *fp;

	sprintf(build, "Build On %s %s", __DATE__, __TIME__);
	fp = fopen("/BUILD", "r");
	if(fp)
	{
		fgets(build, sizeof(build), fp);
		fclose(fp);
	}
	vty_out (vty, "%s\n", build);
}

int cmd_common_init(void);
int cmd_ip_init(void);
int cmd_tool_init(void);

void cmd_parse_init(void)
{
		 cmd_common_init();
     cmd_ip_init();
     cmd_tool_init();
     prime_vty_init();
}

/* VTY shell main routine. */
int main (int argc, char **argv)
{
	char *line;
	int opt;
	int eval_flag = 0;
	int boot_flag = 0;
	char *eval_line = NULL;
	char *config_file = "/etc/config/" PRIME_DEFAULT_CONFIG;
	char *log_file = "/tmp/" PRIME_DEFAULT_LOGFILE;
	int loglevel = PRIME_LOG_DEBUG;

	if(getenv("PRIME_CONFIG"))
		config_file = getenv("PRIME_CONFIG");

  if(getenv("PRIME_LOGFILE"))
		log_file = getenv("PRIME_LOGFILE");

	while (1)
	{
		opt = getopt_long (argc, argv, "be:l:f:c:hv", longopts, 0);
		if (opt == EOF)
			break;
		switch (opt)
		{
			case 0:
				break;
			case 'b':
				boot_flag = 1;
				break;
			case 'e':
				eval_flag = 1;
				eval_line = optarg;
				break;
			case 'h':
				usage (argv[0], 0);
				break;
			case 'c':
				config_file = optarg;
				break;
			case 'l':
				loglevel = atoi(optarg);
				break;
			case 'f':
				log_file = optarg;
				break;
			case 'v':
				printf("Ver:%s %s\n", __DATE__, __TIME__);
				exit(0);
			default:
				usage (argv[0], 1);
				break;
		}
	}

  /* Logging */
  prime_set_logfile(log_file);
	prime_set_loglevel(loglevel);
	prime_enable_log();

  /* Signal and others. */
	signal_init ();

  /* Init config. */
	config_init();

	/* Init the cmd */
	cmd_init();

	/* Init the vtysh */
	vtysh_init_vty ();

	/* Install command and node view */
	cmd_parse_init();

	/* sort the node */
	cmd_sort_node();

	/* If eval mode */
	if (eval_flag)
	{
		vtysh_execute("enable");
		vtysh_execute("config terminal");
		exit(vtysh_execute(eval_line));
	}

	/* Boot startup configuration file. */
	if (boot_flag)
		exit(vtysh_boot_config (config_file));

  in_show_welcome();
	host.config = config_file;
	vtysh_load_config(config_file);

#if 0
  /* Run Daemon with saved configuration */
	//prime_node_main(argc, argv);
	prime_node_main(0, NULL);
#endif

	/* Main command loop. */
	while ((line = vtysh_readline()) != NULL){
		vtysh_execute (line);
  }
	printf ("\n");

	exit (0);
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
