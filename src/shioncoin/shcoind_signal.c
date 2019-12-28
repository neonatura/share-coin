
/*
 * @copyright
 *
 *  Copyright 2014 Brian Burrell
 *
 *  This file is part of Shioncoin.
 *  (https://github.com/neonatura/shioncoin)
 *        
 *  ShionCoin is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version. 
 *
 *  ShionCoin is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ShionCoin.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  @endcopyright
 */  

#include "shcoind.h"
#include <signal.h>

static void shcoind_diag_signal(int sig_num);
static void shcoind_term_signal(int sig_num);


static void shcoind_chld_signal(int sig_num)
{
	/* interrupt timer */
}

void shcoind_signal_init(void)
{

  signal(SIGSEGV, SIG_DFL);

  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

	signal(SIGCHLD, shcoind_chld_signal);

  signal(SIGTERM, shcoind_term_signal);
  signal(SIGQUIT, shcoind_term_signal);
  signal(SIGINT, shcoind_term_signal);

  signal(SIGUSR1, shcoind_diag_signal);
}

#define DEFAULT_SHUTDOWN_CYCLES 4
int _shutdown_timer;
void set_shutdown_timer(void)
{

  if (_shutdown_timer == 0)
    _shutdown_timer = DEFAULT_SHUTDOWN_CYCLES;

}

static void shcoind_term_signal(int sig_num)
{
  signal(sig_num, SIG_DFL);

  set_shutdown_timer();
}

static void shcoind_diag_signal(int sig_num)
{

  /* re-apply signal */
  signal(sig_num, shcoind_diag_signal);

  descriptor_list_print();

}
