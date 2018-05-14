#include "sql_operate.h"
#include "sql_table.h"
#include "m1_common_log.h"

/*prtotype*************************************************************************************************************/
static int modify(sql_operate* d);
/*variable*************************************************************************************************************/
const sql_methods sql_modify= {
	modify,
	NULL,
	NULL
};

static int modify(sql_operate* d)
{
	M1_LOG_INFO("modify\n");

	extern sqlite3* db;

	return sql_step(db, d->sql);
}