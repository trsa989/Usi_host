/**
 * \file
 *
 * \brief CMD Tool file.
 *
 * Copyright (c) 2020 Microchip Technology Inc. and its subsidiaries.
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

#include "command.h"
#include "vtysh_config.h"
#include <ctype.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <unistd.h>

DEFUN(config_ping,
        config_ping_cmd,
        "ping WORD",
        "Send echo messages\n"
        "Ping destination address or hostname\n")
{
    cmd_execute_system_command ("ping", 1, argv);
    return CMD_SUCCESS;
}

DEFUN(config_ping_count,
        config_ping_cmd_count,
        "ping WORD NUM",
        "send count echo messages\n"
        "Ping destination address or hostname\n"
        "Stop after sending NUM ECHO_REQUEST packets\n")
{
    char *myargv[5];

    if(!isdigit(argv[1][0]))
    {
        vty_out(vty, "Invalid number '%s', Please input the count number\n", argv[1]);
        return CMD_ERR_NOTHING_TODO;
    }
    myargv[0] = argv[0];
    myargv[1] = "-c";
    myargv[2] = argv[1];
    cmd_execute_system_command ("ping", 3, myargv);
    return CMD_SUCCESS;
}

DEFUN(config_traceroute,
        config_traceroute_cmd,
        "traceroute WORD",
        "Trace route to destination\n"
        "Trace route to destination address or hostname\n")
{
    cmd_execute_system_command ("traceroute", 1, argv);
    return CMD_SUCCESS;
}

DEFUN(config_telnet,
        config_telnet_cmd,
        "telnet WORD",
        "Open a telnet connection\n"
        "IP address or hostname of a remote system\n")
{
    cmd_execute_system_command ("telnet", 1, argv);
    return CMD_SUCCESS;
}

DEFUN(config_telnet_port,
        config_telnet_port_cmd,
        "telnet WORD PORT",
        "Open a telnet connection\n"
        "IP address or hostname of a remote system\n"
        "TCP Port number\n")
{
    cmd_execute_system_command ("telnet", 2, argv);
    return CMD_SUCCESS;
}

DEFUN(config_reboot,
        config_reboot_cmd,
        "reboot",
        "Reboot the system\n"
        "Reboot the system\n")
{
    vty_out(vty, "System will reboot, please waiting...\n");
    system("sleep 1; reboot -f &");
    exit(1);
    return CMD_SUCCESS;
}

DEFUN(config_poweroff,
        config_poweroff_cmd,
        "poweroff",
        "Power off the system\n"
        "Power off the system\n")
{
    vty_out(vty, "System power off!\n");
    system("sleep 1; poweroff &");
    return CMD_SUCCESS;
}

DEFUN(config_tftp,
        config_tftp_cmd,
        "tftp (get|put) WORD WORD A.B.C.D",
        "begin a tftp session\n"
        "get file from remote\n"
        "put file to remote\n"
        "local file name\n"
        "remote file name\n"
        "tftp server address\n")
{
    char *myargv[10];
    char localfile[256];

    if(strchr(argv[1], '/'))
    {
        vty_out(vty, "invalid local file name.please remove the '/' character.\n");
        return CMD_ERR_NOTHING_TODO;
    }
    sprintf(localfile, "%s/%s", CONFIG_DIR, argv[1]);
    if(strcmp(argv[0], "put") == 0)
        myargv[0] = "-p";
    else
        myargv[0] = "-g";
    myargv[1] = "-l";
    myargv[2] = localfile;
    myargv[3] = "-r";
    myargv[4] = argv[2];
    myargv[5] = argv[3];
    return cmd_execute_system_command("tftp", 6, myargv);;
}

DEFUN (service_telnet_server,
        service_telnet_server_cmd,
        "service telnet-server",
        "Set up telnet daemon service\n"
        "Enable telnet daemon\n")
{
    config_del_line(config_top, "service telnet-server");
    config_add_line(config_top, "service telnet-server");

    ENSURE_CONFIG(vty);

    return cmd_execute_system_command("telnetd", 0, NULL);
}

DEFUN (no_service_telnet_server,
        no_service_telnet_server_cmd,
        "no service telnet-server",
        NO_STR
        "Disable telnet server service\n"
        "Disable telnet server\n")
{
    char *myargv[2];

    config_del_line(config_top, "service telnet-server");
    myargv[0] = "telnetd";
    return cmd_execute_system_command("killall", 1, myargv);
}

DEFUN (service_ssh_server,
        service_ssh_server_cmd,
        "service ssh-server",
        "Set up ssh daemon service\n"
        "Enable ssh daemon\n")
{
    config_del_line(config_top, "service ssh-server");
    config_add_line(config_top, "service ssh-server");
    ENSURE_CONFIG(vty);
    return cmd_execute_system_command("dropbear", 0, NULL);
}

DEFUN (no_service_ssh_server,
        no_service_ssh_server_cmd,
        "no service ssh-server",
        NO_STR
        "Disable ssh server service\n"
        "Disable ssh server\n")
{
    char *myargv[2];

    config_del_line(config_top, "service ssh-server");
    myargv[0] = "dropbear";
    return cmd_execute_system_command("killall", 1, myargv);
}

#include <sys/vfs.h>    /* or <sys/statfs.h> */

DEFUN(config_dir,
        config_dir_cmd,
        "dir",
        "list the flash files\n")
{
    char *myargv[10];
    struct statfs buf;

    myargv[0] = "-Alh";
    myargv[1] = CONFIG_DIR;
    cmd_execute_system_command ("ls", 2, myargv);

    memset(&buf, 0, sizeof(buf));
    if(statfs(CONFIG_DIR, &buf) < 0)
    {
        myargv[0] = "-h";
        myargv[1] = CONFIG_DIR;
        return cmd_execute_system_command ("df", 2, myargv);
    }
    vty_out(vty, "\nTotal size:%dM free:%dM\n",
            (buf.f_bsize >> 10) * (buf.f_blocks >> 10),
            (buf.f_bsize >> 10) * (buf.f_bavail >> 10)
           );
    return 0;
}

#include <sys/vfs.h>    /* or <sys/statfs.h> */

DEFUN(config_sysinfo,
        config_sysinfo_cmd,
        "show sysinfo",
        SHOW_STR
        "display the system info\n")
{
    vty_out(vty, "please waiting ...\n");
    system("top -n 1 | head -n 10");
    return 0;
}

DEFUN(config_delete,
        config_delete_cmd,
        "delete WORD",
        "delete the flash files\n"
        "the file need delete\n")
{
    char *myargv[10];
    char filename[1024];

    sprintf(filename, "%s/%s", CONFIG_DIR, argv[0]);
    myargv[0] = "-f";
    myargv[1] = filename;
    return cmd_execute_system_command ("rm", 2, myargv);
}

DEFUN(config_logging,
        config_logging_cmd,
        "logging (memory|WORD)",
        "config the logging\n"
        "logging to the memory only\n"
        "logging to the memory and host.the ip address or ip:port,eg 192.168.1.1:514")
{
    char *myargv[10];

    config_del_line_byleft(config_top, "logging");


    if(strcmp(argv[0], "memory") == 0)
    {
        config_add_line(config_top, "logging memory");
        ENSURE_CONFIG(vty);

    	system("killall klogd > /dev/null 2>&1");
	system("klogd");

        myargv[0] = "syslogd";
        cmd_execute_system_command("killall", 1, myargv);
        return cmd_execute_system_command("syslogd", 0, myargv);
    }
    else
    {
        config_add_line(config_top, "logging %s", argv[0]);
        ENSURE_CONFIG(vty);

    	system("killall klogd > /dev/null 2>&1");
	system("klogd");

        myargv[0] = "syslogd";
        cmd_execute_system_command("killall", 1, myargv);
        myargv[0] = "-L";
        myargv[1] = "-R";
        myargv[2] = argv[0];
        return cmd_execute_system_command("syslogd", 3, myargv);
    }
}

DEFUN(config_no_logging,
        config_no_logging_cmd,
        "no logging",
        NO_STR
        "Disable the logging\n")
{
    char *myargv[10];
    config_del_line_byleft(config_top, "logging");

    system("killall klogd > /dev/null 2>&1");
    myargv[0] = "syslogd";
    return cmd_execute_system_command("killall", 1, myargv);
}

DEFUN(config_date,
        config_date_cmd,
        "date WORD WORD",
        "Set the date\n"
        "the date YYYY-MM-DD\n"
        "the time HH:MM:SS"
     )
{
    char *myargv[10];
    char date[256];

    myargv[0] = "-s";
    sprintf(date, "%s %s", argv[0], argv[1]);
    myargv[1] = date;
    cmd_execute_system_command("date", 2, myargv);
    myargv[0] = "-w";
    return cmd_execute_system_command("hwclock", 1, myargv);
}

/* Show date. */
DEFUN (show_date,
        show_date_cmd,
        "show date",
        SHOW_STR
        "Displays the current date\n")
{
    return cmd_execute_system_command("date", 0, argv);
}

DEFUN (show_uptime,
       show_uptime_cmd,
       "show uptime",
       SHOW_STR
       "Displays uptime\n")
{

    int updays, uphours, upminutes;
    struct sysinfo info;

    sysinfo(&info);

    vty_out(vty,"System wake up ");
    updays = (int) info.uptime / (60*60*24);
    if (updays)
        vty_out(vty,"%d day%s, ", updays, (updays != 1) ? "s" : "");
    upminutes = (int) info.uptime / 60;
    uphours = (upminutes / 60) % 24;
    upminutes %= 60;
    if (uphours)
        vty_out(vty,"%2d:%02d\n", uphours, upminutes);
    else
        vty_out(vty,"%d min\n", upminutes);

    return CMD_SUCCESS;
}

/* Show version. */
DEFUN (show_logging,
        show_logging_cmd,
        "show logging",
        SHOW_STR
        "Displays the logging information\n")
{
    char *myargv[10];

    if(access("/var/log/messages", R_OK) != 0)
    {
        vty_out(vty, "Please first enable the logging\n");
        return CMD_ERR_NOTHING_TODO;
    }
    myargv[0] = "-n";
    myargv[1] = "1000";
    myargv[2] = "/var/log/messages";
    return cmd_execute_system_command("tail", 3, myargv);
}

/*****************************end******************************************/

int cmd_tool_init()
{
    /* Each node's basic commands. */
    cmd_install_element (VIEW_NODE, &config_ping_cmd);
    cmd_install_element (VIEW_NODE, &config_ping_cmd_count);
    cmd_install_element (VIEW_NODE, &config_traceroute_cmd);
    cmd_install_element (VIEW_NODE, &config_telnet_cmd);
    cmd_install_element (VIEW_NODE, &config_telnet_port_cmd);

    cmd_install_element (ENABLE_NODE, &config_ping_cmd);
    cmd_install_element (ENABLE_NODE, &config_ping_cmd_count);
    cmd_install_element (ENABLE_NODE, &config_traceroute_cmd);
    cmd_install_element (ENABLE_NODE, &config_telnet_cmd);
    cmd_install_element (ENABLE_NODE, &config_telnet_port_cmd);
    cmd_install_element (ENABLE_NODE, &config_reboot_cmd);
    cmd_install_element (ENABLE_NODE, &config_poweroff_cmd);
    cmd_install_element (ENABLE_NODE, &config_tftp_cmd);
    cmd_install_element (ENABLE_NODE, &config_dir_cmd);
    cmd_install_element (ENABLE_NODE, &config_delete_cmd);
    cmd_install_element (ENABLE_NODE, &show_logging_cmd);
    cmd_install_element (ENABLE_NODE, &config_sysinfo_cmd);
    cmd_install_element (ENABLE_NODE, &show_date_cmd);

    cmd_install_element (CONFIG_NODE, &config_ping_cmd);
    cmd_install_element (CONFIG_NODE, &config_ping_cmd_count);
    cmd_install_element (CONFIG_NODE, &config_traceroute_cmd);
    cmd_install_element (CONFIG_NODE, &config_telnet_cmd);
    cmd_install_element (CONFIG_NODE, &config_telnet_port_cmd);
    cmd_install_element (CONFIG_NODE, &service_telnet_server_cmd);
    cmd_install_element (CONFIG_NODE, &no_service_telnet_server_cmd);
    cmd_install_element (CONFIG_NODE, &service_ssh_server_cmd);
    cmd_install_element (CONFIG_NODE, &no_service_ssh_server_cmd);
    cmd_install_element (CONFIG_NODE, &config_logging_cmd);
    cmd_install_element (CONFIG_NODE, &config_no_logging_cmd);
    cmd_install_element (CONFIG_NODE, &show_logging_cmd);
    cmd_install_element (CONFIG_NODE, &config_date_cmd);
    cmd_install_element (CONFIG_NODE, &show_date_cmd);
    cmd_install_element (CONFIG_NODE, &show_uptime_cmd);

    return 0;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
