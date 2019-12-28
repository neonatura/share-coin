
/*
 * @copyright
 *
 *  Copyright 2016 Brian Burrell
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


#ifdef linux
#include <stdio.h>
#endif


int bc_table_hash(bc_hash_t hash)
{
  int i;
  uint32_t *p;
  uint32_t val;

  val = 0;
  p = (uint32_t *)hash;
  for (i = 0; i < 8; i++)
    val += p[i];

  return ((int)(val % BC_TABLE_SIZE));
}

bcpos_t *bc_table_pos(bc_t *bc, bc_hash_t hash)
{
  bcpos_t *tab;
  int idx;

  tab = (bcpos_t *)bc->tab_map.raw;
  if (!tab)
    return (NULL);

  idx = bc_table_hash(hash);
  return (&tab[idx]);
}

static int _bc_table_get(bc_t *bc, bc_hash_t hash, bcpos_t *ret_pos)
{
  bcpos_t *pos_p;
  int err;
  int i;

  err = bc_table_open(bc);
  if (err)
    return (err);

  if (ret_pos)
    *ret_pos = BC_TABLE_NULL_POS; 

  pos_p = bc_table_pos(bc, hash);
  if (!pos_p)
    return (ERR_INVAL);
  if (*pos_p == BC_TABLE_SEARCH_POS)
    return (ERR_SRCH);
  if (*pos_p == BC_TABLE_NULL_POS)
    return (ERR_NOENT); /* no record exists */

  if (ret_pos) {
    *ret_pos = *pos_p;
  }

  return (0);  
}

int bc_table_get(bc_t *bc, bc_hash_t hash, bcpos_t *ret_pos)
{
  int err;

  if (!bc_lock(bc))
		return (ERR_NOLCK);
  err = _bc_table_get(bc, hash, ret_pos);
  bc_unlock(bc);

  return (err);
}

static int _bc_table_set(bc_t *bc, bc_hash_t hash, bcpos_t pos)
{
  bcpos_t *pos_p;
  uint8_t a, b;
  int err;

  if (!bc || pos >= BC_TABLE_POS_MASK)
    return (ERR_INVAL);

  err = bc_table_open(bc);
  if (err)
    return (err);

  pos_p = bc_table_pos(bc, hash);
  if (!pos_p)
    return (ERR_INVAL);

  if (*pos_p < pos || *pos_p >= BC_TABLE_POS_MASK) {
    /* retain highest index position for hash entry */
    *pos_p = pos;
  }

  return (0);
}

int bc_table_set(bc_t *bc, bc_hash_t hash, bcpos_t pos)
{
  int err;

  if (!bc_lock(bc))
		return (ERR_NOLCK);
  err = _bc_table_set(bc, hash, pos);
  bc_unlock(bc);

  return (err);
}

static int _bc_table_unset(bc_t *bc, bc_hash_t hash)
{
  bcpos_t *pos_p;
  uint8_t a, b;
  int err;

  if (!bc)
    return (ERR_INVAL);

  err = bc_table_open(bc);
  if (err)
    return (err);

  pos_p = bc_table_pos(bc, hash);
  if (!pos_p)
    return (ERR_INVAL);

  *pos_p = BC_TABLE_NULL_POS;

  return (0);
}

int bc_table_unset(bc_t *bc, bc_hash_t hash)
{
  int err;

  if (!bc_lock(bc))
		return (ERR_NOLCK);
  err = _bc_table_unset(bc, hash);
  bc_unlock(bc);

  return (err);
}

static int _bc_table_reset(bc_t *bc, bc_hash_t hash)
{
  bcpos_t *pos_p;
  uint8_t a, b;
  int err;

  if (!bc)
    return (ERR_INVAL);

  err = bc_table_open(bc);
  if (err)
    return (err);

  pos_p = bc_table_pos(bc, hash);
  if (!pos_p)
    return (ERR_INVAL);

  
  *pos_p = BC_TABLE_SEARCH_POS;

  return (0);
}

int bc_table_reset(bc_t *bc, bc_hash_t hash)
{
  int err;

  if (!bc_lock(bc))
		return (ERR_NOLCK);
  err = _bc_table_reset(bc, hash);
  bc_unlock(bc);

  return (err);
}

static int _bc_table_open(bc_t *bc)
{
  char errbuf[256];
  int err;

  if (!bc)
    return (ERR_INVAL);

  if (bc->tab_map.fd != 0) {
    return (0);
  }

  /* set map file extension */
  strncpy(bc->tab_map.ext, BC_TABLE_EXTENSION, sizeof(bc->tab_map.ext)-1);

  err = bc_map_open(bc, &bc->tab_map);
  if (err) {
    sprintf(errbuf, "bc_table_open: map open error: %s.", sherrstr(err));
    shcoind_log(errbuf);
    return (err);
  }

  err = bc_map_alloc(bc, &bc->tab_map, (BC_TABLE_SIZE * sizeof(bcpos_t)));
  if (err) {
    sprintf(errbuf, "bc_table_open: map alloc error: %s.", sherrstr(err));
    shcoind_log(errbuf);
    return (err);
  }

  return (0);
}

int bc_table_open(bc_t *bc)
{
  int err;

  if (!bc_lock(bc))
		return (ERR_NOLCK);
  err = _bc_table_open(bc);
  bc_unlock(bc);

  return (err);
}

static void _bc_table_close(bc_t *bc)
{

  if (bc->tab_map.fd != 0) {
    bc_map_close(&bc->tab_map);
  }
  
}

void bc_table_close(bc_t *bc)
{

  if (bc_lock(bc)) {
		_bc_table_close(bc);
		bc_unlock(bc);
	}

}

static int _bc_table_clear(bc_t *bc)
{
  int err;

  err = bc_table_open(bc);
  if (err)
    return (err);

  /* mark entire hash-map table as 'search required'. */
  memset(bc->tab_map.raw, '\000', bc->tab_map.size - sizeof(bc_hdr_t));

  return (0);
}

int bc_table_clear(bc_t *bc)
{
  int err;

  if (!bc_lock(bc))
		return (ERR_NOLCK);
  err = _bc_table_clear(bc);
  bc_unlock(bc);

  return (err);
}

