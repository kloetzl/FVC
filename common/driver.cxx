/**
 * @file
 *
 * This is the main file. It contains functions to parse the commandline arguments,
 * read files etc.
 * 
 * @brief The main file
 * @author Fabian Klötzl
 
 * @section License
 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <iostream>
#include <ostream>
#include <iterator>

#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include <err.h>

#include "esa.h"

#ifdef _OPENMP
#include <omp.h>
#endif

/* Global variables */
int FLAGS = 0;
int THREADS = 1;

void usage(void);
void version(void);

int main( int argc, char *argv[]){
	int c;
	int version_flag = 0;
	
	static struct option long_options[] = {
		{"version", no_argument, &version_flag, 1},
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'},
		{"subject", no_argument, NULL, 's'},
		{0,0,0,0}
	};

	std::vector<std::string> subjects;
	
	// parse arguments
	while( 1 ){
	
		int option_index = 0;
		
		c = getopt_long( argc, argv, "vht:s:", long_options, &option_index);
		
		if( c == -1){
			break;
		}

		switch (c){
			case 0:
				break;
			case 'h':
				usage();
				break;
			case 'v':
				break;
			case 's':
				{
					std::stringstream buffer;
					std::ifstream in(optarg, std::ios::in );

					if( !in.good()) {
						warn("%s", optarg);
						continue;
					}

					buffer << in.rdbuf();
					subjects.push_back(std::move(buffer.str()));
				}
				break;
#ifdef _OPENMP
			case 't':
				{
					errno = 0;
					char *end;
					long unsigned int threads = strtoul( optarg, &end, 10);

					if( errno || end == optarg || *end != '\0'){
						warnx(
							"Expected a number for -t argument, but '%s' was given. "
							"Ignoring -t argument.", optarg
						);
						break;
					}

					if( threads > (long unsigned int) omp_get_num_procs() ){
						warnx(
							"The number of threads to be used, is greater then the number of available processors; "
							"Ignoring -t %lu argument.", threads
						);
						break;
					}

					THREADS = threads;
				}
				break;
#endif
			case '?': /* intentional fall-through */
			default:
				usage();
				break;
		}
	}
	
	if( version_flag ){
		version();
	}

	argc -= optind;
	argv += optind;

	std::vector<std::string> queries;

	// parse all files
	for( ; argc; argv++, argc--){
		std::stringstream buffer;
		std::ifstream in(*argv, std::ios::in );

		if( !in.good()) {
			warn("%s", *argv);
			continue;
		}

		buffer << in.rdbuf();
		queries.push_back(std::move(buffer.str()));
	}

	for (const auto& str : subjects){
		std::cout << str.size() << std::endl;

		esa_t E;
		esa_init(&E, str.data(), str.size());

		for( const auto& Q: queries){
			all_matches(&E,Q);
		}

		esa_free(&E);
	}

	return 0;
}

void usage(void){
	const char str[]= {
		"Usage: fvc [-t INT] FILES...\n"
		"\tFILES... can be any sequence of files. If no files are supplied, stdin is used instead.\n"
		"Options:\n"
#ifdef _OPENMP
		"  -t <INT>          The number of threads to be used; default: 1\n"
#endif
		"  -h, --help        Display this help and exit\n"
		"      --version     Output version information and acknowledgments\n"
	};

	std::cout << str;
	exit(EXIT_SUCCESS);
}

void version(void){
	const char str[]= {
		"fvc \n"
		"Copyright (C) 2014 Fabian Klötzl\n"
	};

	std::cout << str;
	exit(EXIT_SUCCESS);
}
