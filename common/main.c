// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <env.h>
#include <version.h>
#include <asm/arch/at91_sfr.h>
#include <asm/arch/at91_pit.h>
#include <asm/arch/hardware.h>

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
	char *p;

	p = env_get("preboot");
	if (p != NULL) {
		int prev = 0;

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			prev = disable_ctrlc(1); /* disable Ctrl-C checking */

		run_command_list(p, -1, 0);

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			disable_ctrlc(prev);	/* restore Ctrl-C checking */
	}
}

static void set_cpu_serial(void)
{
	int ret;
	struct atmel_sfr *sfr = (struct atmel_sfr *)ATMEL_BASE_SFR;
	
	ret = env_get_hex("sn0", 0);
	if(ret != sfr->sn0)
		goto save;

	printf("Serial Number 0 Register: 0x%08x\n", ret);

	ret = env_get_hex("sn1", 0);
	if(ret != sfr->sn1)
		goto save;

	printf("Serial Number 1 Register: 0x%08x\n", ret);

	return;

save:
	ret = env_set_hex("sn0", sfr->sn0);
	if(ret){
		printf("Cannot set varialbe sn0: 0x%08x\n", sfr->sn0);
		return;
	}

	ret = env_set_hex("sn1", sfr->sn1);
	if(ret){
		printf("Cannot set varialbe sn1: 0x%08x\n", sfr->sn1);
		return;
	}
	printf("Saving CPU serial number.\n");
	env_save();
}

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;
	
	set_cpu_serial();

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

	if (IS_ENABLED(CONFIG_VERSION_VARIABLE))
		env_set("ver", version_string);  /* set version variable */

	cli_init();

	if (IS_ENABLED(CONFIG_USE_PREBOOT))
		run_preboot_environment_command();

	if (IS_ENABLED(CONFIG_UPDATE_TFTP))
		update_tftp(0UL, NULL, NULL);

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
