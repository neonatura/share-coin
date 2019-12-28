
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

#ifndef __BLOCKCHAIN__BC_ARCH_H__
#define __BLOCKCHAIN__BC_ARCH_H__


int bc_arch_open(bc_t *bc);

void bc_arch_close(bc_t *bc);

/**
 * Add a new "archived record" to the chain.
 */
int bc_arch_add(bc_t *bc, bc_idx_t *arch);

uint32_t bc_arch_crc(bc_hash_t hash);

int bc_arch_get(bc_t *bc, bcpos_t pos, bc_idx_t *ret_arch);

/**
 * The next record index available in the database. 
 */
int bc_arch_next(bc_t *bc, bcpos_t *pos_p);

int bc_arch_set(bc_t *bc, bcpos_t pos, bc_idx_t *arch);

int bc_arch_find(bc_t *bc, bc_hash_t hash, bc_idx_t *ret_arch, bcsize_t *ret_pos);



#endif /* ndef __BLOCKCHAIN__BC_ARCH_H__ */

