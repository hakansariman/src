/*
 * Copyright (c) 1997, 1998, 1999 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#include "kadm5_locl.h"

RCSID("$KTH: get_s.c,v 1.13 2000/06/19 16:11:31 joda Exp $");

kadm5_ret_t
kadm5_s_get_principal(void *server_handle, 
		      krb5_principal princ, 
		      kadm5_principal_ent_t out, 
		      u_int32_t mask)
{
    kadm5_server_context *context = server_handle;
    kadm5_ret_t ret;
    hdb_entry ent;
    
    ent.principal = princ;
    ret = context->db->open(context->context, context->db, O_RDONLY, 0);
    if(ret)
	return ret;
    ret = context->db->fetch(context->context, context->db, 
			     HDB_F_DECRYPT, &ent);
    context->db->close(context->context, context->db);
    if(ret)
	return _kadm5_error_code(ret);

    memset(out, 0, sizeof(*out));
    if(mask & KADM5_PRINCIPAL)
	ret  = krb5_copy_principal(context->context, ent.principal, 
				   &out->principal);
    if(ret)
	goto out;
    if(mask & KADM5_PRINC_EXPIRE_TIME && ent.valid_end)
	out->princ_expire_time = *ent.valid_end;
    if(mask & KADM5_PW_EXPIRATION && ent.pw_end)
	out->pw_expiration = *ent.pw_end;
    if(mask & KADM5_LAST_PWD_CHANGE)
	/* XXX implement */;
    if(mask & KADM5_ATTRIBUTES){
	out->attributes |= ent.flags.postdate ? 0 : KRB5_KDB_DISALLOW_POSTDATED;
	out->attributes |= ent.flags.forwardable ? 0 : KRB5_KDB_DISALLOW_FORWARDABLE;
	out->attributes |= ent.flags.initial ? KRB5_KDB_DISALLOW_TGT_BASED : 0;
	out->attributes |= ent.flags.renewable ? 0 : KRB5_KDB_DISALLOW_RENEWABLE;
	out->attributes |= ent.flags.proxiable ? 0 : KRB5_KDB_DISALLOW_PROXIABLE;
	out->attributes |= ent.flags.invalid ? KRB5_KDB_DISALLOW_ALL_TIX : 0;
	out->attributes |= ent.flags.require_preauth ? KRB5_KDB_REQUIRES_PRE_AUTH : 0;
	out->attributes |= ent.flags.server ? 0 : KRB5_KDB_DISALLOW_SVR;
	out->attributes |= ent.flags.change_pw ? KRB5_KDB_PWCHANGE_SERVICE : 0;
    }
    if(mask & KADM5_MAX_LIFE) {
	if(ent.max_life)
	    out->max_life = *ent.max_life;
	else
	    out->max_life = INT_MAX;
    }
    if(mask & KADM5_MOD_TIME) {
	if(ent.modified_by)
	    out->mod_date = ent.modified_by->time;
	else
	    out->mod_date = ent.created_by.time;
    }
    if(mask & KADM5_MOD_NAME) {
	if(ent.modified_by) {
	    if (ent.modified_by->principal != NULL)
		ret = krb5_copy_principal(context->context, 
					  ent.modified_by->principal,
					  &out->mod_name);
	} else if(ent.created_by.principal != NULL)
	    ret = krb5_copy_principal(context->context, 
				      ent.created_by.principal,
				      &out->mod_name);
	else
	    out->mod_name = NULL;
    }
    if(ret)
	goto out;

    if(mask & KADM5_KVNO)
	out->kvno = ent.kvno;
    if(mask & KADM5_MKVNO) {
	int n;
	out->mkvno = 0; /* XXX */
	for(n = 0; n < ent.keys.len; n++)
	    if(ent.keys.val[n].mkvno) {
		out->mkvno = *ent.keys.val[n].mkvno; /* XXX this isn't right */
		break;
	    }
    }
    if(mask & KADM5_AUX_ATTRIBUTES)
	/* XXX implement */;
    if(mask & KADM5_POLICY)
	out->policy = NULL;
    if(mask & KADM5_MAX_RLIFE) {
	if(ent.max_renew)
	    out->max_renewable_life = *ent.max_renew;
	else
	    out->max_renewable_life = INT_MAX;
    }
    if(mask & KADM5_LAST_SUCCESS)
	/* XXX implement */;
    if(mask & KADM5_LAST_FAILED)
	/* XXX implement */;
    if(mask & KADM5_FAIL_AUTH_COUNT)
	/* XXX implement */;
    if(mask & KADM5_KEY_DATA){
	int i;
	Key *key;
	krb5_key_data *kd;
	krb5_salt salt;
	krb5_data *sp;
	krb5_get_pw_salt(context->context, ent.principal, &salt);
	out->key_data = malloc(ent.keys.len * sizeof(*out->key_data));
	for(i = 0; i < ent.keys.len; i++){
	    key = &ent.keys.val[i];
	    kd = &out->key_data[i];
	    kd->key_data_ver = 2;
	    kd->key_data_kvno = ent.kvno;
	    kd->key_data_type[0] = key->key.keytype;
	    if(key->salt)
		kd->key_data_type[1] = key->salt->type;
	    else
		kd->key_data_type[1] = KRB5_PADATA_PW_SALT;
	    /* setup key */
	    kd->key_data_length[0] = key->key.keyvalue.length;
	    kd->key_data_contents[0] = malloc(kd->key_data_length[0]);
	    if(kd->key_data_contents[0] == NULL){
		ret = ENOMEM;
		break;
	    }
	    memcpy(kd->key_data_contents[0], key->key.keyvalue.data, 
		   kd->key_data_length[0]);
	    /* setup salt */
	    if(key->salt)
		sp = &key->salt->salt;
	    else
		sp = &salt.saltvalue;
	    kd->key_data_length[1] = sp->length;
	    kd->key_data_contents[1] = malloc(kd->key_data_length[1]);
	    if(kd->key_data_length[1] != 0
	       && kd->key_data_contents[1] == NULL) {
		memset(kd->key_data_contents[0], 0, kd->key_data_length[0]);
		ret = ENOMEM;
		break;
	    }
	    memcpy(kd->key_data_contents[1], sp->data, kd->key_data_length[1]);
	    out->n_key_data = i + 1;
	}
	krb5_free_salt(context->context, salt);
    }
    if(ret){
	kadm5_free_principal_ent(context, out);
	goto out;
    }
    if(mask & KADM5_TL_DATA)
	/* XXX implement */;
out:
    hdb_free_entry(context->context, &ent);

    return _kadm5_error_code(ret);
}
