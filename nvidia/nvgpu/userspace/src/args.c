/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <unit/core.h>
#include <unit/args.h>
#include <unit/io.h>

static struct option core_opts[] = {
	{ "help",		0, NULL, 'h' },
	{ "verbose",		0, NULL, 'v' },
	{ "quiet",		0, NULL, 'q' },
	{ "no-color",		0, NULL, 'C' },
	{ "nvtest",		0, NULL, 'n' },
	{ "is-qnx",		0, NULL, 'Q' },
	{ "unit-load-path",	1, NULL, 'L' },
	{ "driver-load-path",	1, NULL, 'K' },
	{ "num-threads",	1, NULL, 'j' },
	{ "test-level",		1, NULL, 't' },
	{ "debug",		0, NULL, 'd' },
	{ "required",		0, NULL, 'r' },
	{ NULL,			0, NULL,  0  }
};

static const char *core_opts_str = "hvqCnQL:K:j:t:dr:";

void core_print_help(struct unit_fw *fw)
{
	const char **line, *help_msg[] = {
"NvGpu Unit Testing FW. Basic usage\n",
"\n",
"  $ nvgpu_unit [options] <unit>\n",
"\n",
"Basic usage consists of one or more options and a particular unit test to\n",
"execute.\n",
"\n",
"Available options are as follows:\n",
"\n",
"  -h, --help             Print this help message and exit.\n",
"  -v, --verbose          Increment the verbosity level. Can be specified\n",
"                         multiple times.\n",
"  -q, --quiet            Set the verbose level back to 0.\n",
"  -C, --no-color         Disable color printing; for example, if writing\n",
"                         output to a file the color escape sequences will\n",
"                         corrupt that file.\n",
"  -n, --nvtest           Enable nvtest-formatted output results\n",
"  -Q, --is-qnx           QNX specific tests\n",
"  -L, --unit-load-path <PATH>\n",
"                         Path to where the unit test libraries reside.\n",
"  -K, --driver-load-path <PATH>\n",
"                         Path to driver library.\n",
"  -j, --num-threads <COUNT>\n",
"                         Number of threads to use while running all tests.\n",
"  -t, --test-level <LEVEL>\n",
"                         Test plan level. 0=L0, 1=L1. default: 1\n",
"  -d, --debug            Disable signal handling to facilitate debug of",
"                         crashes.\n",
"  -r, --required <FILE>  Path to a file with a list of required tests to\n"
"                         check if all were executed.\n",
"\n",
"Note: mandatory arguments to long arguments are mandatory for short\n",
"arguments as well.\n",
NULL
	};

	line = help_msg;
	while (*line != NULL) {
		core_msg(fw, "%s", *line);
		line++;
	}
}

static void set_arg_defaults(struct unit_fw_args *args)
{
	args->driver_load_path = DEFAULT_ARG_DRIVER_LOAD_PATH;
	args->unit_load_path = DEFAULT_ARG_UNIT_LOAD_PATH;
	args->thread_count = 1;
	args->test_lvl = TEST_PLAN_MAX;
	args->required_tests_file = NULL;
}

/*
 * Parse command line arguments.
 */
int core_parse_args(struct unit_fw *fw, int argc, char **argv)
{
	int c, opt_index;
	struct unit_fw_args *args;

	args = malloc(sizeof(*args));
	if (!args)
		return -1;

	memset(args, 0, sizeof(*args));
	set_arg_defaults(args);

	args->binary_name = strrchr(argv[0], '/');
	if (args->binary_name == NULL) {
		/* no slash, so use the whole name */
		args->binary_name = argv[0];
	} else {
		/* move past the slash */
		args->binary_name++;
	}

	fw->args = args;

	while (1) {
		c = getopt_long(argc, argv,
				core_opts_str, core_opts, &opt_index);

		if (c == -1)
			break;

		switch (c) {
		case 'h':
			args->help = true;
			break;
		case 'v':
			args->verbose_lvl += 1;
			break;
		case 'q':
			args->verbose_lvl = 0;
			break;
		case 'C':
			args->no_color = true;
			break;
		case 'n':
			args->nvtest = true;
			break;
		case 'L':
			args->unit_load_path = optarg;
			break;
		case 'K':
			args->driver_load_path = optarg;
			break;
		case 'j':
			args->thread_count = strtol(optarg, NULL, 10);
			if (args->thread_count == 0) {
				core_err(fw, "Invalid number of threads\n");
				return -1;
			}
			break;
		case 'Q':
			args->is_qnx = true;
			break;
		case 't':
			args->test_lvl = strtol(optarg, NULL, 10);
			if (args->test_lvl > TEST_PLAN_MAX) {
				core_err(fw, "Invalid test plan level\n");
				return -1;
			}
			break;
		case 'd':
			args->debug = true;
			break;
		case 'r':
			args->required_tests_file = optarg;
			break;
		case '?':
			args->help = true;
			return -1;
		default:
			core_err(fw, "bug?!\n");
			return -1;
		}
	}

	/*
	 * If there is an extra argument after the command-line options, then
	 * it is a unit test name that need to be specifically run.
	 */
	if (optind < argc) {
		args->unit_to_run = argv[optind];
	}

	return 0;
}
