
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


void f_shcoind_log(int err_code, const char *tag, const char *text, const char *src_fname, long src_line)
{
  static shbuf_t *buff;
  char fname[PATH_MAX+1];
  char origin[256];
  char *date_str;
  char buf[256];
  size_t len;

  if (!err_code && !opt_num(OPT_DEBUG)) {
    return;
  }

  if (!buff)
    buff = shbuf_init();

  if (tag) {
    shbuf_catstr(buff, (char *)tag);
    shbuf_catstr(buff, ": ");
  }
	if (err_code && err_code != SHERR_INFO) {
		char *err_str = error_str(err_code);
		//shbuf_catstr(buff, "error: ");
		shbuf_catstr(buff, err_str);
		shbuf_catstr(buff, ": ");
	}
  if (text) {
    len = strlen(text);
    if (*text && text[strlen(text)-1] == '\n')
      len--;
    shbuf_cat(buff, text, len);
  }
  if (src_fname && src_line) {
    char *ptr = strrchr(src_fname, '/');
    if (!ptr)
      strncpy(fname, src_fname, sizeof(fname)-1);
    else
      strncpy(fname, ptr+1, sizeof(fname)-1);

    sprintf(origin, " (%s:%ld)", fname, src_line);
    shbuf_catstr(buff, origin);
  }

  if (err_code && err_code != SHERR_INFO) {
    shlog(SHLOG_ERROR, 0, shbuf_data(buff));
    //shlog(SHLOG_ERROR, err_code, shbuf_data(buff));
  } else {
    shlog(SHLOG_INFO, 0, shbuf_data(buff));
  }

  shbuf_clear(buff);
}

void timing_init(char *tag, shtime_t *stamp_p)
{
  
	/* mark the time when we started timing. */
  *stamp_p = shtime();

}

void timing_term(int ifaceIndex, char *tag, shtime_t *stamp_p)
{
  shtime_t stamp = *stamp_p;
  double diff = shtime_diff(stamp, shtime());
  char buf[256];

  if (diff >= 0.5) { /* 1/2 a second */
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf)-1,
				"TIMING[%s]: total %-2.2f seconds.", tag, diff);
    if (!ifaceIndex)
      shcoind_log(buf);
    else
      unet_log(ifaceIndex, buf);
  }

}

void f_shcoind_log_net(const char *iface, const char *addr, const char *tag, size_t size, const char *src_fname, long src_line)
{
	static shbuf_t *buff;
	static FILE *net_fl;
	static time_t last_t;
	char fname[PATH_MAX+1];
	char origin[256];
	char *date_str;
	char buf[256];
	size_t len;
	time_t now;

	if (!buff)
		buff = shbuf_init();

	now = time(NULL);
	strftime(buf, sizeof(buf)-1, "[%D %T] ", localtime(&now));
	shbuf_catstr(buff, buf);
	shbuf_catstr(buff, iface);
	shbuf_catstr(buff, " ");

	if (addr) {
		shbuf_catstr(buff, (char *)addr);
		shbuf_catstr(buff, ": ");
	}
	if (tag) {
		shbuf_catstr(buff, (char *)tag);
		shbuf_catstr(buff, " ");
	}
	sprintf(buf, "%ub", (unsigned int)size);
	shbuf_catstr(buff, (char *)buf);

#if 0
	if (src_fname && src_line) {
		char *ptr = strrchr(src_fname, '/');
		if (!ptr)
			strncpy(fname, src_fname, sizeof(fname)-1);
		else
			strncpy(fname, ptr+1, sizeof(fname)-1);

		sprintf(origin, "(%s:%ld)", fname, src_line);
		shbuf_catstr(buff, origin);
	}
#endif

	if (!net_fl) {
		char path[PATH_MAX+1];
		sprintf(path, "%snet.log", shlog_path("shcoind"));
		net_fl = fopen(path, "ab");
	}
	if (net_fl)
		fprintf(net_fl, "%s\n", shbuf_data(buff));

	if (last_t != now) {
		fflush(net_fl);
		last_t = now;
	}

	shbuf_clear(buff);
}

