#include       	<common.h>
#include		<libbridge.h>
#include 		<sys/types.h>
#include		<errno.h>
#include		"ofpd.h"
#include 		"ofp_cmd.h"

const cmd_list_t *cmd_lookup(const char *cmd);
STATUS ofp_cmd2mailbox(U8 *mu, int mulen);
int8_t cmd_parse(char ofp_cli[], int *count, char argument[4][64]);
static int cmd_addbr(int argc, char argv[4][64]);
static int cmd_delbr(int argc, char argv[4][64]);
static int cmd_addif(int argc, char argv[4][64]);
static int cmd_delif(int argc, char argv[4][64]);
static int cmd_show_flows(int argc, char argv[4][64]);

static const cmd_list_t cmd_list[] = {
	{ 1, "addbr", cmd_addbr, "<bridge>\t\tadd bridge" },
	{ 1, "delbr", cmd_delbr, "<bridge>\t\tdelete bridge" },
	{ 2, "addif", cmd_addif, 
	  "<bridge> <device>\tadd interface to bridge" },
	{ 2, "delif", cmd_delif,
	  "<bridge> <device>\tdelete interface from bridge" },
	{ 1, "show", cmd_show_flows, "flows\t\tshow the flow table" },
	/*{ 3, "hairpin", br_cmd_hairpin,
	  "<bridge> <port> {on|off}\tturn hairpin on/off" },
	{ 2, "setageing", br_cmd_setageing,
	  "<bridge> <time>\t\tset ageing time" },
	{ 2, "setbridgeprio", br_cmd_setbridgeprio,
	  "<bridge> <prio>\t\tset bridge priority" },
	{ 2, "setfd", br_cmd_setfd,
	  "<bridge> <time>\t\tset bridge forward delay" },
	{ 2, "sethello", br_cmd_sethello,
	  "<bridge> <time>\t\tset hello time" },
	{ 2, "setmaxage", br_cmd_setmaxage,
	  "<bridge> <time>\t\tset max message age" },
	{ 3, "setpathcost", br_cmd_setpathcost, 
	  "<bridge> <port> <cost>\tset path cost" },
	{ 3, "setportprio", br_cmd_setportprio,
	  "<bridge> <port> <prio>\tset port priority" },
	{ 0, "show", br_cmd_show,
	  "[ <bridge> ]\t\tshow a list of bridges" },
	{ 1, "showmacs", br_cmd_showmacs, 
	  "<bridge>\t\tshow a list of mac addrs"},
	{ 1, "showstp", br_cmd_showstp, 
	  "<bridge>\t\tshow bridge stp info"},
	{ 2, "stp", br_cmd_stp,
	  "<bridge> {on|off}\tturn stp on/off" },*/
};

void ofp_cmd(void)
{
	char ofp_cli[256];
	int argue_count;
	const cmd_list_t *cmd;
	char argument[4][64];

	for(;;) {
		printf("Simple_OF_sw> ");
		if (fgets(ofp_cli,256,stdin) == NULL)
			continue;
		if (cmd_parse(ofp_cli,&argue_count,argument) < 0)
			continue;
		if (strcmp(*argument,"\0") == 0)
			continue;
		if ((cmd = cmd_lookup(*argument)) == NULL) {
			fprintf(stderr, "cmd not found [%s]\n", *argument);
			continue;
		}
		if (argue_count < cmd->nargs + 1) {
			printf("Incorrect number of arguments for command\n");
			printf("Usage: %s %s\n", cmd->name, cmd->help);
			continue;
		}
		if (cmd->func(argue_count,argument) < 0)
			continue;
	}
}

const cmd_list_t *cmd_lookup(const char *cmd)
{
	for (int i=0; i<sizeof(cmd_list)/sizeof(cmd_list[0]); i++) {
		if (!strcmp(cmd,cmd_list[i].name))
			return &cmd_list[i];
	}

	return NULL;
}

int8_t cmd_parse(char ofp_cli[], int *count, char argument[4][64])
{
 	char  *str = NULL,

  	*delim = " ",
  	*token = NULL,
   	*rest_of_str = NULL;

	*count = 0;
 	str = strdup(ofp_cli);

 	//start strtok_r

 	token = strtok_r(str,delim,&rest_of_str);
	for(int i=0; i<=strlen(token); i++) {
		if (*(token+i) == '\n')
			*(token+i) = '\0';
	}
 	(*count)++;
	strncpy(*argument,token,64);

 	while(1) {
  		if ((token = strtok_r(rest_of_str,delim,&rest_of_str)) == NULL)
			break;
		for(int i=0; i<strlen(token)+1; i++) {
			if (*(token+i) == '\n')
				*(token+i) = '\0';
		}
		argument++;
		strncpy(*argument,token,64);
  		(*count)++;
 	}
 	free(str);

	return 0;
}

static int cmd_show_flows(int argc, char argv[4][64])
{
	cli_2_ofp_t cli_2_ofp;

	cli_2_ofp.opcode = SHOW_FLOW;
	if (ofp_cmd2mailbox((U8 *)&cli_2_ofp,sizeof(cli_2_ofp_t)) == TRUE)
		return 0;
	return 1;
}

static int cmd_addbr(int argc, char argv[4][64])
{
	int err;
	cli_2_ofp_t cli_2_ofp;

	switch (err = br_add_bridge(argv[1])) {
	case 0:
		cli_2_ofp.opcode = ADD_BR;
		strncpy(cli_2_ofp.brname,argv[1],64);
		if (ofp_cmd2mailbox((U8 *)&cli_2_ofp,sizeof(cli_2_ofp_t)) == TRUE)
			return 0;
		return 1;

	case EEXIST:
		fprintf(stderr,	"device %s already exists; can't create "
			"bridge with the same name\n", argv[1]);
		return 1;
	default:
		fprintf(stderr, "add bridge failed: %s\n",
			strerror(err));
		return 1;
	}

	return 0;
}

static int cmd_delbr(int argc, char argv[4][64])
{
	int err;
	cli_2_ofp_t cli_2_ofp;

	switch (err = br_del_bridge(argv[1])) {
	case 0:
		cli_2_ofp.opcode = DEL_BR;
		strncpy(cli_2_ofp.brname,argv[1],64);
		if (ofp_cmd2mailbox((U8 *)&cli_2_ofp,sizeof(cli_2_ofp_t)) == TRUE)
			return 0;
		return 1;

	case ENXIO:
		fprintf(stderr, "bridge %s doesn't exist; can't delete it\n",
			argv[1]);
		return 1;

	case EBUSY:
		fprintf(stderr, "bridge %s is still up; can't delete it\n",
			argv[1]);
		return 1;

	default:
		fprintf(stderr, "can't delete bridge %s: %s\n",
			argv[1], strerror(err));
		return 1;
	}

	return 0;
}

static int cmd_addif(int argc, char argv[4][64])
{
	const char *brname;
	cli_2_ofp_t cli_2_ofp;
	int err;

	argc -= 2;
	brname = *++argv;

	while (argc-- > 0) {
		const char *ifname = *++argv;
		err = br_add_interface(brname, ifname);

		switch(err) {
		case 0:
			cli_2_ofp.opcode = ADD_IF;
			strncpy(cli_2_ofp.brname,brname,64);
			strncpy(cli_2_ofp.ifname,ifname,64);
			if (ofp_cmd2mailbox((U8 *)&cli_2_ofp,sizeof(cli_2_ofp_t)) == TRUE);
			continue;

		case ENODEV:
			if (if_nametoindex(ifname) == 0)
				fprintf(stderr, "interface %s does not exist!\n", ifname);
			else
				fprintf(stderr, "bridge %s does not exist!\n", brname);
			break;

		case EBUSY:
			fprintf(stderr,	"device %s is already a member of a bridge; "
				"can't enslave it to bridge %s.\n", ifname,
				brname);
			break;

		case ELOOP:
			fprintf(stderr, "device %s is a bridge device itself; "
				"can't enslave a bridge device to a bridge device.\n",
				ifname);
			break;

		default:
			fprintf(stderr, "can't add %s to bridge %s: %s\n",
				ifname, brname, strerror(err));
		}
		return 1;
	}
	return 0;

	return 0;
}

static int cmd_delif(int argc, char argv[4][64])
{
	const char *brname;
	int err;
	cli_2_ofp_t cli_2_ofp;

	argc -= 2;
	brname = *++argv;

	while (argc-- > 0) {
		const char *ifname = *++argv;
		err = br_del_interface(brname, ifname);
		switch (err) {
		case 0:
			cli_2_ofp.opcode = DEL_IF;
			strncpy(cli_2_ofp.brname,argv[1],64);
			strncpy(cli_2_ofp.ifname,ifname,64);
			if (ofp_cmd2mailbox((U8 *)&cli_2_ofp,sizeof(cli_2_ofp_t)) == TRUE);
			continue;

		case ENODEV:
			if (if_nametoindex(ifname) == 0)
				fprintf(stderr, "interface %s does not exist!\n", ifname);
			else
				fprintf(stderr, "bridge %s does not exist!\n", brname);
			break;

		case EINVAL:
			fprintf(stderr, "device %s is not a slave of %s\n",
				ifname, brname);
			break;

		default:
			fprintf(stderr, "can't delete %s from %s: %s\n",
				ifname, brname, strerror(err));
		}
		return 1;
	}
	return 0;

	return 0;
}

/*********************************************************
 * ofp_cmd2mailbox:
 *
 * Input  : mu - incoming ethernet+igmp message unit
 *          mulen - mu length
 *          port - 0's based
 *          sid - need tag ? (Y/N)
 *          prior - if sid=1, then need set this field
 *          vlan - if sid=1, then need set this field
 *
 * return : TRUE or ERROR(-1)
 *********************************************************/
STATUS ofp_cmd2mailbox(U8 *mu, int mulen)
{
	tOFP_MBX mail;

    if (ofpQid == -1) {
		if ((ofpQid=msgget(OFP_Q_KEY,0600|IPC_CREAT)) < 0) {
			printf("send> Oops! ofpQ(key=0x%x) not found\n",OFP_Q_KEY);
   	 	}
	}
	
	if (mulen > ETH_MTU) {
	 	printf("Incoming frame length(%d) is too large at ofp_cmd.c!\n",mulen);
		return ERROR;
	}

	mail.len = mulen;
	memcpy(mail.refp,mu,mulen); /* mail content will be copied into mail queue */
	
	//printf("ofp_send2mailbox(ofp_sock.c %d): mulen=%d\n",__LINE__,mulen);
	mail.type = IPC_EV_TYPE_CLI;
	ipc_sw(ofpQid, &mail, sizeof(mail), -1);
	return TRUE;
}
