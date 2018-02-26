#include "sql_operate.h"
#include "sql_table.h"
#include "m1_common_log.h"

/*prtotype*************************************************************************************************************/
static int delete(sql_operate* d);
/*variable*************************************************************************************************************/
const sql_methods sql_delete= {
	delete,
	NULL,
	NULL
};

static int delete(sql_operate* d)
{
	M1_LOG_INFO("delete\n");

	extern sqlite3* db;

	return sql_step(db, d->sql);
}