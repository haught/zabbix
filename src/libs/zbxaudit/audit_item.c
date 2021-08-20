/*
** Zabbix
** Copyright (C) 2001-2021 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "dbcache.h"

#include "log.h"
#include "audit_item.h"

static int	item_flag_to_resource_type(int flag)
{
	if (ZBX_FLAG_DISCOVERY_NORMAL == flag || ZBX_FLAG_DISCOVERY_CREATED == flag)
	{
		return AUDIT_RESOURCE_ITEM;
	}
	else if (ZBX_FLAG_DISCOVERY_PROTOTYPE == flag)
	{
		return AUDIT_RESOURCE_ITEM_PROTOTYPE;
	}
	else if (ZBX_FLAG_DISCOVERY_RULE == flag)
	{
		return AUDIT_RESOURCE_DISCOVERY_RULE;
	}
	else
	{
		zabbix_log(LOG_LEVEL_DEBUG, "unexpected audit detected: ->%d<-", flag);
		THIS_SHOULD_NEVER_HAPPEN;
		exit(EXIT_FAILURE);
	}
}

#define	PREPARE_AUDIT_ITEM_CREATE(resource, audit_resource_flag)						\
void	zbx_audit_##resource##_create_entry(int audit_action, zbx_uint64_t itemid, const char *name)		\
{														\
	zbx_audit_entry_t	local_audit_item_entry, **found_audit_item_entry;				\
	zbx_audit_entry_t	*local_audit_item_entry_x = &local_audit_item_entry;				\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	local_audit_item_entry.id = itemid;									\
														\
	found_audit_item_entry = (zbx_audit_entry_t**)zbx_hashset_search(zbx_get_audit_hashset(),		\
			&(local_audit_item_entry_x));								\
	if (NULL == found_audit_item_entry)									\
	{													\
		zbx_audit_entry_t	*local_audit_item_entry_insert;						\
														\
		local_audit_item_entry_insert = (zbx_audit_entry_t*)zbx_malloc(NULL, sizeof(zbx_audit_entry_t));\
		local_audit_item_entry_insert->id = itemid;							\
		local_audit_item_entry_insert->name = zbx_strdup(NULL, name);					\
		local_audit_item_entry_insert->audit_action = audit_action;					\
		local_audit_item_entry_insert->resource_type = audit_resource_flag;				\
		zbx_json_init(&(local_audit_item_entry_insert->details_json), ZBX_JSON_STAT_BUF_LEN);		\
		zbx_hashset_insert(zbx_get_audit_hashset(), &local_audit_item_entry_insert,			\
				sizeof(local_audit_item_entry_insert));						\
	}													\
}														\

PREPARE_AUDIT_ITEM_CREATE(item, AUDIT_RESOURCE_ITEM)
#undef PREPARE_AUDIT_ITEM_CREATE

#define ONLY_ITEM (AUDIT_RESOURCE_ITEM == resource_type)
#define ONLY_ITEM_PROTOTYPE (AUDIT_RESOURCE_ITEM_PROTOTYPE == resource_type)
#define ONLY_LLD_RULE (AUDIT_RESOURCE_DISCOVERY_RULE == resource_type)

void	zbx_audit_item_update_json_add_data(zbx_uint64_t itemid, const zbx_template_item_t *item, zbx_uint64_t hostid)
{
	int	resource_type;

	RETURN_IF_AUDIT_OFF();

	resource_type = item_flag_to_resource_type(item->flags);

#define ONLY_ITEM_AND_ITEM_PROTOTYPE (AUDIT_RESOURCE_ITEM == resource_type || \
		AUDIT_RESOURCE_ITEM_PROTOTYPE == resource_type)
#define IT_OR_ITP(s) ONLY_ITEM ? "item."#s : (ONLY_ITEM_PROTOTYPE ? "itemprototype."#s : "discoveryrule."#s)
#define ADD_JSON_S(x)	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, IT_OR_ITP(x), item->x)
#define ADD_JSON_UI(x)	zbx_audit_update_json_append_uint64(itemid, AUDIT_DETAILS_ACTION_ADD, IT_OR_ITP(x), item->x)
	zbx_audit_update_json_append_uint64(itemid, AUDIT_DETAILS_ACTION_ADD, IT_OR_ITP(itemid), itemid);
	ADD_JSON_S(delay);
	zbx_audit_update_json_append_uint64(itemid, AUDIT_DETAILS_ACTION_ADD, IT_OR_ITP(hostid), hostid);
	/* ruleid is REQUIRED for item prototype */
	ADD_JSON_UI(interfaceid);
	ADD_JSON_S(key); // API HAS 'key_' , but SQL 'key'
	ADD_JSON_S(name);
	ADD_JSON_UI(type);
	ADD_JSON_S(url);
	if ONLY_ITEM_AND_ITEM_PROTOTYPE ADD_JSON_UI(value_type);
	ADD_JSON_UI(allow_traps);
	ADD_JSON_UI(authtype);
	ADD_JSON_S(description);
	/* error - only for item and LLD RULE */
	if ONLY_ITEM ADD_JSON_UI(flags);
	ADD_JSON_UI(follow_redirects);
	ADD_JSON_S(headers);
	if ONLY_ITEM_AND_ITEM_PROTOTYPE ADD_JSON_S(history);
	ADD_JSON_S(http_proxy);
	if ONLY_ITEM ADD_JSON_UI(inventory_link);
	ADD_JSON_S(ipmi_sensor);
	ADD_JSON_S(jmx_endpoint);
	if ONLY_LLD_RULE ADD_JSON_S(lifetime);
	/* lastclock - only for item */
	/* last ns - only for item */
	/* lastvalue - only for item */
	if ONLY_ITEM_AND_ITEM_PROTOTYPE ADD_JSON_S(logtimefmt);
	ADD_JSON_UI(master_itemid);
	ADD_JSON_UI(output_format);
	ADD_JSON_S(params);
	/* parameters , handled later - for both item and item prototype and LLD RULE */
	ADD_JSON_S(password);
	ADD_JSON_UI(post_type);
	ADD_JSON_S(posts);
	/* prevvalue - only for item */
	ADD_JSON_S(privatekey);
	ADD_JSON_S(publickey);
	ADD_JSON_S(query_fields);
	ADD_JSON_UI(request_method);
	ADD_JSON_UI(retrieve_mode);
	ADD_JSON_S(snmp_oid);
	ADD_JSON_S(ssl_cert_file);
	ADD_JSON_S(ssl_key_file);
	ADD_JSON_S(ssl_key_password);
	/* state - only for item and LLD RULE */
	ADD_JSON_UI(status);
	ADD_JSON_S(status_codes);
	ADD_JSON_UI(templateid);
	ADD_JSON_S(timeout);
	ADD_JSON_S(trapper_hosts);
	if ONLY_ITEM_AND_ITEM_PROTOTYPE ADD_JSON_S(trends);
	if ONLY_ITEM_AND_ITEM_PROTOTYPE ADD_JSON_S(units);
	ADD_JSON_S(username);
	if ONLY_ITEM_AND_ITEM_PROTOTYPE ADD_JSON_UI(valuemapid);
	ADD_JSON_UI(verify_host);
	ADD_JSON_UI(verify_peer);
	/* discover - only for item */
	/* ITEM API FINISHED */

	/* application - handled later
	preprocessing - handled later */

	if ONLY_LLD_RULE
	{
		ADD_JSON_S(formula);
		ADD_JSON_UI(evaltype);
		ADD_JSON_UI(discover);
	}
#undef ADD_JSON_UI
#undef ADD_JSON_S
#undef IT_OR_ITP
}

#define IT_OR_ITP(s) ONLY_ITEM ? "item."#s : (ONLY_ITEM_PROTOTYPE ? "itemprototype."#s : "discoveryrule."#s)
#define PREPARE_AUDIT_ITEM_UPDATE(resource, type1, type2)							\
void	zbx_audit_item_update_json_update_##resource(zbx_uint64_t itemid, int flags,				\
		type1 resource##_old, type1 resource##_new)							\
{														\
	int	resource_type;											\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	resource_type = item_flag_to_resource_type(flags);							\
	zbx_audit_update_json_update_##type2(itemid, IT_OR_ITP(resource), resource##_old, resource##_new);	\
}

PREPARE_AUDIT_ITEM_UPDATE(interfaceid,		zbx_uint64_t,	uint64)
PREPARE_AUDIT_ITEM_UPDATE(templateid,		zbx_uint64_t,	uint64)
PREPARE_AUDIT_ITEM_UPDATE(name,			const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(type,			int,		int)
PREPARE_AUDIT_ITEM_UPDATE(value_type,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(delay,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(history,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(trends,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(status,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(trapper_hosts,	const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(units,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(formula,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(logtimefmt,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(valuemapid,		zbx_uint64_t,	uint64)
PREPARE_AUDIT_ITEM_UPDATE(params,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(ipmi_sensor,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(snmp_oid,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(authtype,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(username,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(password,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(publickey,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(privatekey,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(flags,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(description,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(inventory_link,	int,		int)
PREPARE_AUDIT_ITEM_UPDATE(lifetime,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(evaltype,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(jmx_endpoint,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(master_itemid,	zbx_uint64_t,	uint64)
PREPARE_AUDIT_ITEM_UPDATE(timeout,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(url,			const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(query_fields,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(posts,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(status_codes,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(follow_redirects,	int,		int)
PREPARE_AUDIT_ITEM_UPDATE(redirects,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(post_type,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(http_proxy,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(headers,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(retrieve_mode,	int,		int)
PREPARE_AUDIT_ITEM_UPDATE(request_method,	int,		int)
PREPARE_AUDIT_ITEM_UPDATE(output_format,	int,		int)
PREPARE_AUDIT_ITEM_UPDATE(ssl_cert_file,	const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(ssl_key_file,		const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(ssl_key_password,	const char*,	string)
PREPARE_AUDIT_ITEM_UPDATE(verify_peer,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(verify_host,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(allow_traps,		int,		int)
PREPARE_AUDIT_ITEM_UPDATE(discover,		int,		int)
#undef PREPARE_AUDIT_ITEM_UPDATE

#undef ONLY_ITEM
#undef ONLY_ITEM_PROTOTYPE
#undef ONLY_LLD_RULE
#undef IT_OR_ITP

void	zbx_audit_discovery_rule_update_json_add_overrides_conditions(zbx_uint64_t itemid,
		zbx_uint64_t item_conditionid, zbx_uint64_t op, const char *macro, const char *value)
{
	char	audit_key[AUDIT_DETAILS_KEY_LEN], audit_key_operator[AUDIT_DETAILS_KEY_LEN],
		audit_key_macro[AUDIT_DETAILS_KEY_LEN], audit_key_value[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key, sizeof(audit_key),
			"discoveryrule.overrides[" ZBX_FS_UI64 "].filter.conditions", item_conditionid);
	zbx_snprintf(audit_key_operator, sizeof(audit_key_operator),
			"discoveryrule.overrides[" ZBX_FS_UI64 "].filter.conditions.operator", item_conditionid);
	zbx_snprintf(audit_key_macro, sizeof(audit_key_macro),
			"discoveryrule.overrides[" ZBX_FS_UI64 "].filter.conditions.macro", item_conditionid);
	zbx_snprintf(audit_key_value, sizeof(audit_key_value),
			"discoveryrule.overrides[" ZBX_FS_UI64 "].filter.conditions.value", item_conditionid);

	zbx_audit_update_json_append_no_value(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key);
	zbx_audit_update_json_append_uint64(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_operator, op);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_macro, macro);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_value, value);
}

void	zbx_audit_discovery_rule_update_json_update_filter_conditions_create_entry(zbx_uint64_t itemid,
		zbx_uint64_t item_conditionid)
{
	char	audit_key[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key, sizeof(audit_key),
			"discoveryrule.overrides[" ZBX_FS_UI64 "].filter.conditions", item_conditionid);

	zbx_audit_update_json_append_no_value(itemid, AUDIT_DETAILS_ACTION_UPDATE, audit_key);
}

#define PREPARE_AUDIT_DISCOVERY_RULE_UPDATE(resource, type1, type2)						\
void	zbx_audit_discovery_rule_update_json_update_filter_conditions_##resource(zbx_uint64_t itemid,		\
		zbx_uint64_t item_conditionid, type1 resource##_old, type1 resource##_new)			\
{														\
	char	audit_key_##resource[AUDIT_DETAILS_KEY_LEN];							\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	zbx_snprintf(audit_key_##resource, sizeof(audit_key_##resource),					\
			"discoveryrule.overrides[" ZBX_FS_UI64 "].filter.conditions."#resource,			\
			item_conditionid);									\
														\
	zbx_audit_update_json_update_##type2(itemid, audit_key_##resource, resource##_old, resource##_new);	\
}
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE(operator, int, int)
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE(macro, const char*, string)
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE(value, const char*, string)
#undef PREPARE_AUDIT_DISCOVERY_RULE_UPDATE

void	zbx_audit_discovery_rule_update_json_delete_overrides_conditions(zbx_uint64_t itemid,
		zbx_uint64_t item_conditionid)
{
	char	buf[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(buf, sizeof(buf), "discoveryrule.overrides[" ZBX_FS_UI64 "]", item_conditionid);

	zbx_audit_update_json_delete(itemid, AUDIT_DETAILS_ACTION_DELETE, buf);
}

void	zbx_audit_discovery_rule_update_json_add_discovery_rule_preproc(zbx_uint64_t itemid,
		zbx_uint64_t item_preprocid, int step, int type, const char *params, int error_handler,
		const char *error_handler_params)
{
	char	audit_key[AUDIT_DETAILS_KEY_LEN], audit_key_step[AUDIT_DETAILS_KEY_LEN],
		audit_key_type[AUDIT_DETAILS_KEY_LEN], audit_key_params[AUDIT_DETAILS_KEY_LEN],
		audit_key_error_handler[AUDIT_DETAILS_KEY_LEN], audit_key_error_handler_params[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key, sizeof(audit_key), "discoveryrule.preprocessing[" ZBX_FS_UI64 "]", item_preprocid);
	zbx_snprintf(audit_key_step, sizeof(audit_key_step),
			"discoveryrule.preprocessing[" ZBX_FS_UI64 "].step", item_preprocid);
	zbx_snprintf(audit_key_type, sizeof(audit_key_type),
			"discoveryrule.preprocessing[" ZBX_FS_UI64 "].type", item_preprocid);
	zbx_snprintf(audit_key_params, sizeof(audit_key_params),
			"discoveryrule.preprocessing[" ZBX_FS_UI64 "].params", item_preprocid);
	zbx_snprintf(audit_key_error_handler, sizeof(audit_key_error_handler),
			"discoveryrule.preprocessing[" ZBX_FS_UI64 "].error_handler", item_preprocid);
	zbx_snprintf(audit_key_error_handler_params, sizeof(audit_key_error_handler_params),
			"discoveryrule.preprocessing[" ZBX_FS_UI64 "].error_handler_params", item_preprocid);

	zbx_audit_update_json_append_no_value(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key);
	zbx_audit_update_json_append_int(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_step, step);
	zbx_audit_update_json_append_int(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_type, type);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_params, params);
	zbx_audit_update_json_append_int(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_step, error_handler);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_step, error_handler_params);
}

void	zbx_audit_discovery_rule_update_json_update_discovery_rule_preproc_create_entry(zbx_uint64_t itemid,
		zbx_uint64_t preprocid)
{
	char	audit_key[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key, sizeof(audit_key), "discoveryrule.preprocessing[" ZBX_FS_UI64 "]", preprocid);
	zbx_audit_update_json_append_no_value(itemid, AUDIT_DETAILS_ACTION_UPDATE, audit_key);
}

#define PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_PREPROC(resource, type1, type2)					\
void	zbx_audit_discovery_rule_update_json_update_discovery_rule_preproc_##resource(zbx_uint64_t itemid,	\
		zbx_uint64_t preprocid, type1 resource##_old, type1 resource##_new)				\
{														\
	char	audit_key_##resource[AUDIT_DETAILS_KEY_LEN];							\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	zbx_snprintf(audit_key_##resource, sizeof(audit_key_##resource),					\
			"discoveryrule.preprocessing[" ZBX_FS_UI64 "]."#resource, preprocid);			\
														\
	zbx_audit_update_json_update_##type2(itemid, audit_key_##resource, resource##_old, resource##_new);	\
}
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_PREPROC(type, int, int)
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_PREPROC(params, const char*, string)
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_PREPROC(error_handler, int, int)
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_PREPROC(error_handler_params, const char*, string)
#undef PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_PREPROC

#define ITEM_RESOURCE_KEY_RESOLVE(resource, nested)								\
	if (AUDIT_RESOURCE_ITEM == resource_type)								\
	{													\
		zbx_snprintf(audit_key_##resource, sizeof(audit_key_##resource), "item.parameters[" ZBX_FS_UI64 \
				"]"#nested#resource, item_parameter_id);					\
	}													\
	else if (AUDIT_RESOURCE_ITEM_PROTOTYPE == resource_type)						\
	{													\
		zbx_snprintf(audit_key_##resource, sizeof(audit_key_##resource), "itemprototype.parameters["	\
				ZBX_FS_UI64 "]"#nested#resource, item_parameter_id);				\
	}													\
	else if (AUDIT_RESOURCE_DISCOVERY_RULE == resource_type)						\
	{													\
		zbx_snprintf(audit_key_##resource, sizeof(audit_key_##resource), "discoveryrule.parameters["	\
				ZBX_FS_UI64 "]"#resource, item_parameter_id);					\
	}													\
	else													\
		THIS_SHOULD_NEVER_HAPPEN;

void	zbx_audit_item_update_json_add_params(zbx_uint64_t itemid, int item_flags, zbx_uint64_t item_parameter_id,
		const char *name, const char *value)
{
	int	resource_type;
	char	audit_key_name[AUDIT_DETAILS_KEY_LEN], audit_key_value[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();
	resource_type = item_flag_to_resource_type(item_flags);

	ITEM_RESOURCE_KEY_RESOLVE(name, ".")
	ITEM_RESOURCE_KEY_RESOLVE(value, ".")

	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_name, name);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_value, value);
}

#define PREPARE_AUDIT_ITEM_PARAMS_UPDATE(resource)								\
void	zbx_audit_item_update_json_update_params_##resource(zbx_uint64_t itemid, int item_flags,		\
		zbx_uint64_t item_parameter_id,	const char *resource##_orig, const char *resource)		\
{														\
	int	resource_type;											\
	char	audit_key_##resource[AUDIT_DETAILS_KEY_LEN];							\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	resource_type = item_flag_to_resource_type(item_flags);							\
	ITEM_RESOURCE_KEY_RESOLVE(resource, ".")								\
														\
	zbx_audit_update_json_update_string(itemid, audit_key_##resource, resource##_orig, resource);		\
}

PREPARE_AUDIT_ITEM_PARAMS_UPDATE(name)
PREPARE_AUDIT_ITEM_PARAMS_UPDATE(value)

void	zbx_audit_item_delete_params(zbx_uint64_t itemid, int item_flags, zbx_uint64_t item_parameter_id)
{
	int	resource_type;
	char	audit_key_[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	resource_type = item_flag_to_resource_type(item_flags);

	ITEM_RESOURCE_KEY_RESOLVE(, "")

	zbx_audit_update_json_delete(itemid, AUDIT_DETAILS_ACTION_DELETE, audit_key_);
}

void	zbx_audit_discovery_rule_update_json_add_lld_macro_path(zbx_uint64_t itemid,
		zbx_uint64_t lld_macro_pathid, const char *lld_macro, const char *path)
{
	char	audit_key_lld_macro[AUDIT_DETAILS_KEY_LEN], audit_key_path[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key_lld_macro, sizeof(audit_key_lld_macro),
			"discoveryrule.lld_macro_paths[" ZBX_FS_UI64 "].lld_macro", lld_macro_pathid);
	zbx_snprintf(audit_key_path, sizeof(audit_key_lld_macro),
			"discoveryrule.lld_macro_paths[" ZBX_FS_UI64 "].path", lld_macro_pathid);

	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_lld_macro, lld_macro);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_path, path);
}

#define PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_LLD_MACRO_PATH(resource)						\
void	zbx_audit_discovery_rule_update_json_update_lld_macro_path_##resource(zbx_uint64_t itemid,		\
		zbx_uint64_t lld_macro_pathid, const char *resource##_old, const char *resource##_new)		\
{														\
	char	audit_key_##resource[AUDIT_DETAILS_KEY_LEN];							\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	zbx_snprintf(audit_key_##resource, sizeof(audit_key_##resource),					\
			"discoveryrule.lld_macro_paths[" ZBX_FS_UI64 "]."#resource, lld_macro_pathid);		\
														\
	zbx_audit_update_json_update_string(itemid, audit_key_##resource, resource##_old, resource##_new);	\
}
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_LLD_MACRO_PATH(lld_macro)
PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_LLD_MACRO_PATH(path)
#undef PREPARE_AUDIT_DISCOVERY_RULE_UPDATE_LLD_MACRO_PATH

void	zbx_audit_discovery_rule_update_json_delete_lld_macro_path(zbx_uint64_t itemid,
		zbx_uint64_t lld_macro_pathid)
{
	char	buf[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(buf, sizeof(buf),"discoveryrule.lld_macro_paths[" ZBX_FS_UI64 "]", lld_macro_pathid);

	zbx_audit_update_json_delete(itemid, AUDIT_DETAILS_ACTION_DELETE, buf);
}

void zbx_audit_discovery_rule_update_json_add_lld_override(zbx_uint64_t itemid, zbx_uint64_t overrideid,
		const char *name, int step, int stop)
{
	char	audit_key_name[AUDIT_DETAILS_KEY_LEN], audit_key_step[AUDIT_DETAILS_KEY_LEN],
		audit_key_stop[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key_name, sizeof(audit_key_name), "discoveryrule.overrides[" ZBX_FS_UI64 "].name",
			overrideid);

	zbx_snprintf(audit_key_step, sizeof(audit_key_step), "discoveryrule.overrides[" ZBX_FS_UI64 "].step",
			overrideid);

	zbx_snprintf(audit_key_stop, sizeof(audit_key_stop), "discoveryrule.overrides[" ZBX_FS_UI64 "].stop",
			overrideid);

	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_name, name);
	zbx_audit_update_json_append_int(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_step, step);
	zbx_audit_update_json_append_int(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_stop, stop);
}

void zbx_audit_discovery_rule_update_json_add_lld_override_filter(zbx_uint64_t itemid, zbx_uint64_t overrideid,
		int evaltype, const char *formula)
{
	char	audit_key_evaltype[AUDIT_DETAILS_KEY_LEN], audit_key_formula[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key_evaltype, sizeof(audit_key_evaltype), "discoveryrule.overrides[" ZBX_FS_UI64
			"].filter.evaltype", overrideid);

	zbx_snprintf(audit_key_formula, sizeof(audit_key_formula), "discoveryrule.overrides[" ZBX_FS_UI64
			"].filter.formula", overrideid);

	zbx_audit_update_json_append_int(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_evaltype, evaltype);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_formula, formula);
}

void zbx_audit_discovery_rule_update_json_add_lld_override_condition(zbx_uint64_t itemid, zbx_uint64_t overrideid,
		zbx_uint64_t override_conditionid, int operator, const char *macro, const char *value)
{
	char	audit_key_operator[AUDIT_DETAILS_KEY_LEN], audit_key_macro[AUDIT_DETAILS_KEY_LEN],
		audit_key_value[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key_operator, sizeof(audit_key_operator), "discoveryrule.overrides[" ZBX_FS_UI64
		"].conditions[" ZBX_FS_UI64 "].operator", overrideid, override_conditionid);

	zbx_snprintf(audit_key_macro, sizeof(audit_key_macro), "discoveryrule.overrides[" ZBX_FS_UI64 "].conditions["
			ZBX_FS_UI64 "].macro", overrideid, override_conditionid);

	zbx_snprintf(audit_key_value, sizeof(audit_key_value), "discoveryrule.overrides[" ZBX_FS_UI64 "].conditions["
			ZBX_FS_UI64 "].value", overrideid, override_conditionid);

	zbx_audit_update_json_append_int(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_operator, operator);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_macro, macro);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_value, value);
}

#define PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(resource, type, type2)					\
void	zbx_audit_discovery_rule_update_json_add_lld_override_##resource(zbx_uint64_t itemid,			\
		zbx_uint64_t overrideid, zbx_uint64_t resource##_id, type resource)				\
{														\
	char	buf[AUDIT_DETAILS_KEY_LEN];									\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	zbx_snprintf(buf, sizeof(buf), "discoveryrule.overrides[" ZBX_FS_UI64					\
			"].operations[" ZBX_FS_UI64 "]."#resource, overrideid, resource##_id);			\
														\
	zbx_audit_update_json_append_##type2(itemid, AUDIT_DETAILS_ACTION_ADD, buf, resource);			\
}

PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(opstatus, int, int)
PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(opdiscover, int, int)
PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(opperiod, const char*, string)
PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(optrends, const char*, string)
PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(ophistory, const char*, string)
PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(opseverity, int, int)
PREPARE_AUDIT_DISCOVERY_RULE_OVERRIDE_ADD(opinventory, int, int)

void	zbx_audit_discovery_rule_update_json_add_lld_override_optag(zbx_uint64_t itemid, zbx_uint64_t overrideid,
		zbx_uint64_t lld_override_optagid, const char *tag, const char *value)
{
	char	audit_key_tag[AUDIT_DETAILS_KEY_LEN], audit_key_value[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(audit_key_tag, sizeof(audit_key_tag), "discoveryrule.overrides[" ZBX_FS_UI64
			"].optag[" ZBX_FS_UI64 "].tag", overrideid, lld_override_optagid);

	zbx_snprintf(audit_key_value, sizeof(audit_key_value), "discoveryrule.overrides[" ZBX_FS_UI64
			"].optag[" ZBX_FS_UI64 "].value", overrideid, lld_override_optagid);

	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_tag, tag);
	zbx_audit_update_json_append_string(itemid, AUDIT_DETAILS_ACTION_ADD, audit_key_value, value);
}

void	zbx_audit_discovery_rule_update_json_add_lld_override_optemplate(zbx_uint64_t itemid, zbx_uint64_t overrideid,
		zbx_uint64_t lld_override_optemplateid, zbx_uint64_t templateid)
{
	char	buf[AUDIT_DETAILS_KEY_LEN];

	RETURN_IF_AUDIT_OFF();

	zbx_snprintf(buf, sizeof(buf), "discoveryrule.overrides[" ZBX_FS_UI64
			"].optemplateid[" ZBX_FS_UI64 "].templateid", overrideid, lld_override_optemplateid);

	zbx_audit_update_json_append_uint64(itemid, AUDIT_DETAILS_ACTION_ADD, buf, templateid);
}
